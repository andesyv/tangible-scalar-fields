#include "HapticInteractor.h"
#include "../Viewer.h"

#include <iostream>
#include <format>
#include <chrono>
#include <future>
#include <numeric>

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#include <imgui.h>

#ifdef DHD
#include <dhdc.h>
#endif

using namespace molumes;
namespace chr = std::chrono;

#ifdef FAKE_HAPTIC
int KEY_HORIZONTAL_AXIS = 0;
int KEY_VERTICAL_AXIS = 0;
#endif

template<typename T, typename U>
auto max(T &&a, U &&b) {
    return std::max(a, b);
}

template<typename T, typename ... Ts>
auto max(T &&a, Ts &&... rest) {
    return std::max(a, max(rest...));
}

// returns signed distance to plane (negative is below)
auto point_to_plane(const glm::vec3 &pos, const glm::vec3 &pl_norm, const glm::vec3 &pl_pos) {
    glm::vec3 pl_to_pos = pos - pl_pos;
    return glm::dot(pl_to_pos, pl_norm);
}

// Finds relative uv coords in a plane given the planes tangent and bi-tangent
auto
pos_uvs_in_plane(const glm::vec3 &pos, const glm::vec3 &pl_tan, const glm::vec3 &pl_bitan, const glm::vec2 &pl_dims) {
    const glm::vec3 origo{-pl_dims * 0.5f, 0.f};
    const glm::vec3 dir = pos - origo;
    return glm::vec2{glm::dot(dir, pl_tan) / pl_dims.x, glm::dot(dir, pl_bitan) / pl_dims.y};
}

// returns uv as xy and signed distance as z for a position given a orientation matrix for a plane and it's dimensions
auto relative_pos_coords(const glm::vec3 &pos, const glm::mat4 &pl_mat, const glm::vec2 &pl_dims) {
    return glm::vec3{
            pos_uvs_in_plane(pos, glm::vec3{pl_mat[0]}, glm::vec3{pl_mat[1]}, pl_dims),
            point_to_plane(pos, glm::vec3{pl_mat[2]}, glm::vec3{pl_mat[3]})
    };
}

// Bi-linear interpolation of pixel
glm::vec4 sample_tex(const glm::vec2 &uv, const glm::uvec2 tex_dims, const std::vector<glm::vec4> &tex_data) {
    // Opengl 4.0 Specs: glReadPixels: Pixels are returned in row order from the lowest to the highest row, left to right in each row.
    const auto get_pixel = [tex_dims, &tex_data = std::as_const(tex_data)](
            const glm::uvec2 &coord) -> std::optional<glm::vec4> {
        if (tex_data.empty() || tex_dims.x == 0 || tex_dims.y == 0 || tex_dims.x <= coord.x || tex_dims.y <= coord.y)
            return std::nullopt;
        return std::make_optional(tex_data.at(coord.y * tex_dims.x + coord.x));
    };

    const glm::vec2 pixel_coord = uv * glm::vec2{tex_dims + 1u};
    const glm::vec2 f_pixel_coord = glm::fract(pixel_coord);
    const glm::uvec2 i_pixel_coord = glm::uvec2{pixel_coord - f_pixel_coord};

    const auto aa = get_pixel(i_pixel_coord);
    const auto ba = get_pixel(glm::uvec2{i_pixel_coord.x + 1u, i_pixel_coord.y});
    const auto ab = get_pixel(glm::uvec2{i_pixel_coord.x, i_pixel_coord.y + 1u});
    const auto bb = get_pixel(glm::uvec2{i_pixel_coord.x + 1u, i_pixel_coord.y + 1u});
    if (!aa || !ba || !ab || !bb)
        return glm::vec4{0.};

    return glm::mix(
            glm::mix(*aa, *ba, f_pixel_coord.x),
            glm::mix(*ab, *bb, f_pixel_coord.x),
            f_pixel_coord.y);
}

auto sample_force(const glm::vec3 &pos, const glm::ivec2 &tex_dims, const std::vector<glm::vec4> &tex_data) {
    const glm::mat4 pl_mat{1.0}; // Currently just hardcoding a xy-plane lying in origo

    const auto coords = relative_pos_coords(pos, pl_mat, glm::vec2{2.f});
    // Can't calculate a force outside the bounds of the plane, so return no force
    if (coords.x < 0.f || 1.f < coords.x || coords.y < 0.f || 1.f < coords.y)
        return glm::vec3{0.f};

    const auto value = sample_tex(glm::vec2{coords}, glm::uvec2{tex_dims}, tex_data);
    float dist = coords.z - value.w * 0.01f;
    // If dist is positive, it means we're above the surface = no force applied
    if (0.f < dist)
        return glm::vec3{0.f};

    const auto v_len = glm::length(glm::vec3{value});

    // If sampled tex was 0, value can still be zero.
    return v_len < 0.001f ? glm::vec3{0.f} : glm::vec3{value} * (1.f / v_len); // Clamp to 1 Newton
}

#if defined(DHD) || defined(FAKE_HAPTIC)

void haptic_loop(std::stop_token simulation_should_end, std::atomic<glm::vec3> &global_pos,
                 std::atomic<float> &interaction_bounds, std::atomic<bool> &enable_force,
                 std::atomic<float> &max_force, std::promise<bool> &&setup_results,
                 ReaderChannel<std::pair<glm::ivec2, std::vector<glm::vec4>>> &&normal_tex_channel,
                 std::atomic<glm::mat4> &m_view_mat) {
    glm::dvec3 local_pos{0.0};
    bool force_enabled = false;
    double max_bound = 0.01;
    glm::ivec2 normal_tex_size{0};
    std::vector<glm::vec4> normal_tex_data{};
    constexpr double EPSILON = 0.001;
    auto last_t = chr::steady_clock::now();

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

    for (; !simulation_should_end.stop_requested(); std::this_thread::sleep_for(std::chrono::microseconds{1})) {
        // Query for position (actual rate of querying from hardware is controlled by underlying SDK)
#ifdef DHD
        dhdGetPosition(&local_pos.x, &local_pos.y, &local_pos.z);

        // Update max_bound for transformation calculation:
        max_bound = max(std::abs(local_pos.x), std::abs(local_pos.y), std::abs(local_pos.z), max_bound);
#else
        auto current_t = chr::steady_clock::now();
        constexpr auto MOVE_SPEED = 0.01;
        const auto delta_t =
                static_cast<double>(chr::duration_cast<chr::microseconds>(current_t - last_t).count()) * 0.000001;
        last_t = current_t;
        const auto disp = m_view_mat.load() * glm::vec4{KEY_HORIZONTAL_AXIS, 0., -KEY_VERTICAL_AXIS, 0.0};
        local_pos += glm::dvec3{disp} * delta_t * MOVE_SPEED;
#endif

        // Transform to scene space:
        // Novint Falcon is in 10x10x10cm space = 0.1x0.1x0.1m = [-0.05, 0.05] m^3'
        // right = y, up = z, forward = -x
        const auto scale_mult =
                interaction_bounds.load(std::memory_order_relaxed) / max_bound; // [0, max_bound] -> [0, 10]
        const auto pos = local_pos * scale_mult;

        global_pos.store(pos);

        auto[size, data] = normal_tex_channel.get();
        normal_tex_size = size;
        normal_tex_data = std::move(data);

#ifdef DHD
        // Simulation stuff
        if (!enable_force.load()) {
            if (force_enabled) {
                dhdEnableForce(DHD_OFF);
                force_enabled = false;
                std::cout << "Force disabled!" << std::endl;
            }
            continue;
        }
#endif

        const auto force = glm::dvec3{sample_force(pos, normal_tex_size, normal_tex_data) * max_force.load()};

        if (force_enabled) {
            if (!enable_force.load()) {
#ifdef DHD
                dhdEnableForce(DHD_OFF);
#endif
                force_enabled = false;
                std::cout << "Force disabled!" << std::endl;
                continue;
            }
        } else {
            // Wait to apply force until we've arrived at a safe space
            if (enable_force.load() && glm::length(force) < EPSILON) {
#ifdef DHD
                dhdEnableForce(DHD_ON);
#endif
                force_enabled = true;
                std::cout << "Force enabled!" << std::endl;
            } else
                continue;
        }


#ifdef DHD
        dhdSetForce(force.x, force.y, force.z);
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
                                   ReaderChannel<std::pair<glm::ivec2, std::vector<glm::vec4>>> &&normal_tex_channel)
        : Interactor(viewer) {
#ifdef DHD
    std::cout << std::format("Running dhd SDK version {}", dhdGetSDKVersionStr()) << std::endl;

    /// Possible handling of grip/rotation and other additional Haptic stuff here

#endif

#if defined(DHD) || defined(FAKE_HAPTIC)
    std::promise<bool> setup_results{};
    auto haptics_enabled = setup_results.get_future();
    m_thread = std::jthread{haptic_loop, std::ref(m_haptic_finger_pos), std::ref(m_interaction_bounds),
                            std::ref(m_enable_force), std::ref(m_max_force), std::move(setup_results),
                            std::move(normal_tex_channel), std::ref(m_view_mat)};
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
        m_enable_force.store(!m_enable_force.load());

#ifdef FAKE_HAPTIC
    else if (!m_ctrl) {
        if (key == GLFW_KEY_RIGHT)
            KEY_HORIZONTAL_AXIS += inc - dec;
        else if (key == GLFW_KEY_LEFT)
            KEY_HORIZONTAL_AXIS -= inc - dec;
        else if (key == GLFW_KEY_UP)
            KEY_VERTICAL_AXIS += inc - dec;
        else if (key == GLFW_KEY_DOWN)
            KEY_VERTICAL_AXIS -= inc - dec;
    }
#endif
}

void HapticInteractor::display() {
    Interactor::display();

    if (ImGui::BeginMenu("Haptics")) {
        auto interaction_bounds = m_interaction_bounds.load();
        auto enable_force = m_enable_force.load();
        auto max_force = m_max_force.load();
        if (ImGui::SliderFloat("Interaction bounds", &interaction_bounds, 0.1f, 10.f))
            m_interaction_bounds.store(interaction_bounds);
        if (ImGui::Checkbox("Enable force (F)", &enable_force))
            m_enable_force.store(enable_force);
        if (enable_force)
            if (ImGui::SliderFloat("Max haptic force (Newtons)", &max_force, 0.f, 9.f))
                m_max_force.store(max_force);

        ImGui::EndMenu();
    }

    viewer()->m_haptic_pos = m_haptic_finger_pos.load();
    m_view_mat.store(glm::inverse(viewer()->viewTransform()));

    // Event such that haptic renderer can be disabled on runtime. For instance if the device loses connection
    static auto last_haptic_enabled = m_haptic_enabled;
    if (m_on_haptic_toggle && last_haptic_enabled != m_haptic_enabled) {
        m_on_haptic_toggle(m_haptic_enabled);
        last_haptic_enabled = m_haptic_enabled;
    }
}
