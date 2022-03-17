#include "HapticInteractor.h"
#include "../Viewer.h"
#include "../Utils.h"
#include "../Profile.h"
#include "../DelegateUtils.h"
#include "../Physics.h"

#include <iostream>
#include <format>
#include <chrono>
#include <future>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#include <imgui.h>

#ifdef DHD

#include <dhdc.h>

#endif

using namespace molumes;
namespace chr = std::chrono;
using namespace std::chrono_literals;

#ifdef FAKE_HAPTIC
int KEY_X_AXIS = 0;
int KEY_Y_AXIS = 0;
int KEY_Z_AXIS = 0;
#endif

template<typename T, typename U>
auto max(T &&a, U &&b) {
    return std::max(a, b);
}

template<typename T, typename ... Ts>
auto max(T &&a, Ts &&... rest) {
    return std::max(a, max(rest...));
}

template<glm::length_t L, typename T, glm::qualifier Q>
struct std::formatter<glm::vec<L, T, Q>> : std::formatter<std::string> {
    auto format(const glm::vec<L, T, Q> &v, format_context &ctx) {
        return formatter<string>::format(glm::to_string(v), ctx);
    }
};

std::pair<glm::uvec2, std::vector<glm::vec4>>
HapticInteractor::generate_single_mipmap(glm::uvec2 tex_dims, std::vector<glm::vec4> tex_data) {
    const auto new_dims = tex_dims / 2u;
    // coord.y * tex_dims.x + coord.x
    for (glm::uint y = 0; y < new_dims.y; ++y) {
        for (glm::uint x = 0; x < new_dims.x; ++x) {
            const auto sum = tex_data.at(2 * y * tex_dims.x + x * 2) +
                             tex_data.at(2 * y * tex_dims.x + x * 2 + 1) +
                             tex_data.at((2 * y + 1) * tex_dims.x + x * 2) +
                             tex_data.at((2 * y + 1) * tex_dims.x + x * 2 + 1);
            tex_data.at(y * new_dims.x + x) = sum * 0.25f;
        }
    }
    tex_data.resize(tex_data.size() / 4);
    tex_data.shrink_to_fit();

    return std::make_pair(new_dims, std::move(tex_data));
}

#if defined(DHD) || defined(FAKE_HAPTIC)

class HapticKeyHandler {
public:
    struct State {
        bool enabled{false};
        std::vector<std::function<void(bool)>> changed_events;
    };
private:
    std::map<int, State> m_key_states;

public:
    explicit HapticKeyHandler(std::vector<int> &&key_ids) {
        for (auto id: key_ids)
            m_key_states.emplace(id, State{});
    }

    void add_on_changed_event(int key, std::function<void(bool)> &&func) {
        m_key_states[key].changed_events.push_back(std::move(func));
    }

    [[nodiscard]] bool get(int key) {
        return m_key_states[key].enabled;
    }

    bool operator[](int i) { return get(i); }

    void update() {
        for (auto&[key, state]: m_key_states) {
#ifdef DHD
            const auto btn = dhdGetButton(key);
            if (btn < 0) {
                std::cout
                        << std::format("Failed to query key: {{id: {}, error code: {}, error: {}}}", key, btn,
                                       dhdErrorGetLastStr())
                        << std::endl;
            } else {
                const auto new_enabled = btn == DHD_ON;
                if (state.enabled != new_enabled)
                    for (auto &event: state.changed_events)
                        event(new_enabled);
                state.enabled = new_enabled;
            }
#endif
        }
    }
};

void haptic_loop(const std::stop_token &simulation_should_end, HapticInteractor::HapticParams &haptic_params,
                 std::promise<bool> &&setup_results,
                 ReaderChannel<TextureMipMaps> &&normal_tex_channel) {
    glm::dvec3 local_pos{0.0};
    bool force_enabled = false;
    double max_bound = 0.01;
    TextureMipMaps normal_tex_mip_maps;
    constexpr double EPSILON = 0.001;
    auto last_t = chr::high_resolution_clock::now();
    // right = y, up = z, forward = -x
    // Converts from the Novint falcon's coordinate system (up=z), to our coordinate system (up=y)
    const glm::dmat3 haptic_to_local{
            glm::dvec3{0., 0., 1.},
            glm::dvec3{1., 0., 0.},
            glm::dvec3{0., 1., 0.}
    };
    const auto local_to_haptic = glm::inverse(haptic_to_local);
    Physics physics_simulation;
    HapticKeyHandler key_handler{{0}};
    key_handler.add_on_changed_event(0, [&haptic_params, old_mip_map = 0u](bool enabled) mutable {
        if (enabled) {
            old_mip_map = haptic_params.mip_map_level.load();
            haptic_params.mip_map_level.store(HapticMipMapLevels - 1);
        } else {
            haptic_params.mip_map_level.store(old_mip_map);
        }
    });

#ifdef DHD
    // Initialize haptics device
    if (0 <= dhdOpen()) {
        std::cout << std::format("Haptic device detected: {}", dhdGetSystemName()) << std::endl;
        std::cout << std::format(
                "Device capabilities: base: {}, gripper: {}, wrist: {}, active gripper: {}, active wrist: {}",
                dhdHasBase(), dhdHasGripper(), dhdHasWrist(), dhdHasActiveGripper(), dhdHasActiveWrist()) << std::endl;
        setup_results.set_value(true);
    } else {
        // Early quit haptic loop by returning early out of the function
        setup_results.set_value(false);
        return;
    }
#else
    setup_results.set_value(true);
#endif

    while (!simulation_should_end.stop_requested()) {
        // Query for position (actual rate of querying from hardware is controlled by underlying SDK)
        {
            PROFILE("Haptic - Fetch position and velocity");
#ifdef DHD
            dhdGetPosition(&local_pos.x, &local_pos.y, &local_pos.z);

            // Update max_bound for transformation calculation:
            max_bound = max(std::abs(local_pos.x), std::abs(local_pos.y), std::abs(local_pos.z), max_bound);
            local_pos = haptic_to_local * local_pos;
#else
            auto current_t = chr::steady_clock::now();
            constexpr auto MOVE_SPEED = 0.003;
            const auto delta_t =
                    static_cast<double>(chr::duration_cast<chr::microseconds>(current_t - last_t).count()) * 0.000001;
            last_t = current_t;
            const auto disp = glm::vec3{KEY_X_AXIS, KEY_Y_AXIS, -KEY_Z_AXIS};
            local_pos += glm::dvec3{disp} * delta_t * MOVE_SPEED;
#endif
        }

        {
            PROFILE("Haptic - Query keys");
            key_handler.update();
        }

        glm::dvec3 world_pos;
        {
            PROFILE("Haptic - Transform space");
            // Transform to scene space:
            // Novint Falcon is in 10x10x10cm space = 0.1x0.1x0.1m = [-0.05, 0.05] m^3'

            // Scaling multiplication factor to be applied to local coordinates
            const double scale_mult =
                    haptic_params.interaction_bounds.load(std::memory_order_relaxed) /
                    max_bound; // [0, max_bound] -> [0, 10]

            world_pos = (haptic_params.input_space.load() == 0 ? glm::dmat3{1.0} : haptic_params.view_mat_inv.load()) *
                        (local_pos * scale_mult);
        }

        {
            PROFILE("Haptic - Global pos");
            haptic_params.finger_pos.store(glm::vec3{world_pos});
        }

        {
            PROFILE("Haptic - Fetch normal tex");
            if (normal_tex_channel.has_update())
                normal_tex_mip_maps = normal_tex_channel.get();
        }

        // Simulation stuff

        glm::dvec3 world_force{0.0};
        {
            PROFILE("Haptic - Sample force");
            world_force = physics_simulation.simulate_and_sample_force(haptic_params.surface_force.load(),
                                                                       haptic_params.surface_softness.load(),
                                                                       haptic_params.friction_scale.load(),
                                                                       haptic_params.surface_height_multiplier.load(),
                                                                       static_cast<FrictionMode>(haptic_params.friction_mode.load()),
                                                                       haptic_params.mip_map_level.load(),
                                                                       normal_tex_mip_maps, world_pos,
                                                                       haptic_params.normal_offset.load(),
                                                                       haptic_params.gravity_factor.load(),
                                                                       haptic_params.gradual_surface_accuracy.load()
                                                                       ? std::make_optional(
                                                                               haptic_params.surface_volume_mip_map_count.load())
                                                                       : std::nullopt);
        }

        {
            PROFILE("Haptic - Global force");
            haptic_params.force.store(world_force);
        }

#if defined(DHD) && !defined(NDEBUG)
        static auto last_time = dhdGetTime();
        if (dhdGetTime() - last_time > 3.0) {
            last_time = dhdGetTime();
            std::cout << std::format("Haptic frequency: {:.3f} KHz, current force: {:.3f} N", dhdGetComFreq(),
                                     glm::length(world_force)) << std::endl;
        }
#endif

        if (force_enabled) {
            if (!haptic_params.enable_force.load()) {
#ifdef DHD
                dhdSetForce(0.0, 0.0, 0.0);
                dhdEnableForce(DHD_OFF);
#endif
                force_enabled = false;
                std::cout << "Force disabled!" << std::endl;
            }
        } else {
            // Wait to apply force until we've arrived at a safe space
            if (haptic_params.enable_force.load() && glm::length(world_force) < EPSILON) {
#ifdef DHD
                dhdEnableForce(DHD_ON);
#endif
                force_enabled = true;
                std::cout << "Force enabled!" << std::endl;
            }
        }

        if (!force_enabled)
            continue;


#ifdef DHD
        {
            PROFILE("Haptic - Set force");
            // Convert force from world space into the space of the haptic device
            const auto haptic_force = local_to_haptic * (haptic_params.input_space.load() == 0 ? glm::dmat3{1.0}
                                                                                               : haptic_params.view_mat.load()) *
                                      world_force;
            dhdSetForce(haptic_force.x, haptic_force.y, haptic_force.z);
        }
#endif

#ifdef FAKE_DHD
        std::this_thread::sleep_for(1us);
#endif
    }

#ifdef DHD
    const auto op_result = dhdClose();
    if (op_result < 0)
        std::cout << std::format("Failed to close device (error code: {}, error: {})", op_result, dhdErrorGetLastStr())
                  << std::endl;
#endif
}

#endif // DHD

HapticInteractor::HapticInteractor(Viewer *viewer,
                                   ReaderChannel<TextureMipMaps> &&normal_tex_channel)
        : Interactor(viewer) {
#ifdef DHD
    std::cout << std::format("Running dhd SDK version {}", dhdGetSDKVersionStr()) << std::endl;

    /// Possible handling of grip/rotation and other additional Haptic stuff here

#endif

#if defined(DHD) || defined(FAKE_HAPTIC)
    std::promise<bool> setup_results{};
    auto haptics_enabled = setup_results.get_future();
    m_thread = std::jthread{haptic_loop, std::ref(m_params), std::move(setup_results), std::move(normal_tex_channel)};
    m_haptic_enabled = haptics_enabled.get();
#endif
}

HapticInteractor::~HapticInteractor() {
    if (m_thread.joinable()) {
        m_thread.request_stop();
        m_thread.join();
    }
}

void HapticInteractor::keyEvent(int key, int scancode, int action, int mods) {
    Interactor::keyEvent(key, scancode, action, mods);

    const auto inc = int{action == GLFW_PRESS};
    const auto dec = int{action == GLFW_RELEASE};

    if (key == GLFW_KEY_F && action == GLFW_PRESS)
        m_params.enable_force.store(!m_params.enable_force.load());
    else if (key == GLFW_KEY_U && action == GLFW_PRESS)
        m_params.friction_mode.store(!m_params.friction_mode.load());
    else if (key == GLFW_KEY_G && action == GLFW_PRESS)
        m_params.gravity_factor.store(m_params.gravity_factor.load().has_value() ? std::nullopt : std::make_optional(
                m_ui_gravity_factor_value));
#ifndef NDEBUG
    else if (key == GLFW_KEY_P && action == GLFW_PRESS)
        Profiler::reset_profiler();
#endif

#ifdef FAKE_HAPTIC
    else if (!m_ctrl) {
        if (key == GLFW_KEY_RIGHT)
            KEY_X_AXIS += inc - dec;
        else if (key == GLFW_KEY_LEFT)
            KEY_X_AXIS -= inc - dec;
        else if (key == GLFW_KEY_PAGE_UP)
            KEY_Y_AXIS += inc - dec;
        else if (key == GLFW_KEY_PAGE_DOWN)
            KEY_Y_AXIS -= inc - dec;
        else if (key == GLFW_KEY_UP)
            KEY_Z_AXIS += inc - dec;
        else if (key == GLFW_KEY_DOWN)
            KEY_Z_AXIS -= inc - dec;
    }
#endif
}

void HapticInteractor::display() {
    Interactor::display();

    auto mip_map_level = static_cast<int>(m_params.mip_map_level.load());

    if (ImGui::BeginMenu("Haptics")) {
        auto interaction_bounds = m_params.interaction_bounds.load();
        auto enable_force = m_params.enable_force.load();
        auto normal_force = m_params.surface_force.load();
        auto softness = m_params.surface_softness.load();
        int kernel_type = m_params.sphere_kernel.load() ? 1 : 0;
        m_ui_sphere_kernel_size = m_params.sphere_kernel_radius.load();
        int friction_type = static_cast<int>(m_params.friction_mode.load());
        float friction_scale = m_params.friction_scale.load();
        m_ui_surface_height_multiplier = m_params.surface_height_multiplier.load();
        int input_space = static_cast<int>(m_params.input_space.load());
        bool normal_offset = m_params.normal_offset.load();
        bool gravity_enabled = m_params.gravity_factor.load().has_value();
        int surface_volume_mip_map_count = static_cast<int>(m_params.surface_volume_mip_map_count.load());

        if (ImGui::SliderFloat("Interaction bounds", &interaction_bounds, 0.1f, 10.f))
            m_params.interaction_bounds.store(interaction_bounds);
        ImGui::SliderInt("Mip map level", &mip_map_level, 0, HapticMipMapLevels - 1);

        if (ImGui::SliderFloat("Surface height multiplier", &m_ui_surface_height_multiplier, 0.1f, 10.f)) {
            m_params.surface_height_multiplier.store(m_ui_surface_height_multiplier);
            viewer()->BROADCAST(&HapticInteractor::m_ui_surface_height_multiplier);
        }
        if (ImGui::Checkbox("Enable force (F)", &enable_force))
            m_params.enable_force.store(enable_force);
        if (enable_force) {
            if (ImGui::SliderFloat("Surface force (in Newtons)", &normal_force, 0.f, 9.f))
                m_params.surface_force.store(normal_force);
            if (ImGui::SliderFloat("Soft surface-ness", &softness, 0.f, 0.4f))
                m_params.surface_softness.store(softness);
            if (ImGui::Combo("Kernel type", &kernel_type, "Point\0Sphere"))
                m_params.sphere_kernel.store(kernel_type == 1);
            if (kernel_type == 1 && ImGui::SliderFloat("Kernel radius", &m_ui_sphere_kernel_size, 0.001f, 0.1f)) {
                m_params.sphere_kernel_radius.store(m_ui_sphere_kernel_size);
                viewer()->BROADCAST(&HapticInteractor::m_ui_sphere_kernel_size);
            }
            if (ImGui::Combo("Friction", &friction_type, "None\0Uniform\0Directional"))
                m_params.friction_mode.store(static_cast<unsigned int>(friction_type));
            if (friction_type != 0 && ImGui::SliderFloat("Friction scale", &friction_scale, 0.f, 1.f))
                m_params.friction_scale.store(friction_scale);
            if (ImGui::Checkbox("Gravity", &gravity_enabled) ||
                (gravity_enabled && ImGui::SliderFloat("Gravity factor", &m_ui_gravity_factor_value, 0.f, 10.f)))
                m_params.gravity_factor.store(
                        gravity_enabled ? std::make_optional(m_ui_gravity_factor_value) : std::nullopt);
        }
        if (ImGui::Combo("Input space", &input_space, "XZ-Aligned\0Camera Aligned"))
            m_params.input_space.store(input_space);
        if (ImGui::Checkbox("Surface volume mode", &m_ui_surface_volume_mode)) {
            m_params.gradual_surface_accuracy.store(m_ui_surface_volume_mode);
            viewer()->BROADCAST(&HapticInteractor::m_ui_surface_volume_mode);
        }
        if (m_ui_surface_volume_mode) {
            if (ImGui::SliderInt("Surface volume mip map count", &surface_volume_mip_map_count, 2,
                                 HapticMipMapLevels)) {
                m_params.surface_volume_mip_map_count.store(static_cast<unsigned int>(surface_volume_mip_map_count));
                m_ui_surface_volume_enabled_mip_maps = generate_enabled_mip_maps(surface_volume_mip_map_count);
                viewer()->BROADCAST(&HapticInteractor::m_ui_surface_volume_enabled_mip_maps);
            }
        }
        if (ImGui::Checkbox("Offset normal", &normal_offset)) {
            m_params.normal_offset.store(normal_offset);
        }

        ImGui::EndMenu();
    }

    // Have to check it here because it can be set both from within haptic thread and the UI:
    if (static_cast<unsigned int>(mip_map_level) != m_mip_map_ui_level) {
        m_mip_map_ui_level = static_cast<unsigned int>(mip_map_level);
        m_params.mip_map_level.store(m_mip_map_ui_level);
        viewer()->BROADCAST(&HapticInteractor::m_mip_map_ui_level);
    }

    const auto m = glm::dmat3{viewer()->viewTransform()};
    m_params.view_mat.store(m);
    m_params.view_mat_inv.store(glm::inverse(m));

    m_haptic_global_pos = m_params.finger_pos.load();
    m_haptic_global_force = m_params.force.load();
    viewer()->BROADCAST(&HapticInteractor::m_haptic_global_pos);
    viewer()->BROADCAST(&HapticInteractor::m_haptic_global_force);

    // Event such that haptic renderer can be disabled on runtime. For instance if the device loses connection
    static auto last_haptic_enabled = m_haptic_enabled;
    if (m_on_haptic_toggle && last_haptic_enabled != m_haptic_enabled) {
        m_on_haptic_toggle(m_haptic_enabled);
        last_haptic_enabled = m_haptic_enabled;
    }
}