#include "HapticInteractor.h"
#include "../Viewer.h"
#include "../Utils.h"
#include "../Profile.h"
#include "../DelegateUtils.h"

#include <iostream>
#include <format>
#include <chrono>
#include <future>
#include <numeric>
#include <numbers>

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

template<glm::length_t L, typename T, glm::qualifier Q>
struct std::formatter<glm::vec<L, T, Q>> : std::formatter<std::string> {
    auto format(const glm::vec<L, T, Q> &v, format_context &ctx) {
        return formatter<string>::format(glm::to_string(v), ctx);
    }
};

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

auto pos_uvs_in_plane(const glm::vec3 &pos, const glm::mat4 &pl_mat, const glm::vec2 &pl_dims) {
    return pos_uvs_in_plane(pos, glm::vec3{pl_mat[0]}, glm::vec3{pl_mat[1]}, pl_dims);
}

// returns uv as xy and signed distance as z for a position given a orientation matrix for a plane and it's dimensions
auto relative_pos_coords(const glm::vec3 &pos, const glm::mat4 &pl_mat, const glm::vec2 &pl_dims) {
    return glm::vec3{
            pos_uvs_in_plane(pos, glm::vec3{pl_mat[0]}, glm::vec3{pl_mat[1]}, pl_dims),
            point_to_plane(pos, glm::vec3{pl_mat[2]}, glm::vec3{pl_mat[3]})
    };
}

std::optional<glm::vec2> opt_pos_uvs_in_plane(const glm::vec3 &pos, const glm::mat4 &pl_mat, const glm::vec2 &pl_dims) {
    const auto coords = pos_uvs_in_plane(pos, glm::vec3{pl_mat[0]}, glm::vec3{pl_mat[1]}, pl_dims);
    return (coords.x < 0.f || 1.f < coords.x || coords.y < 0.f || 1.f < coords.y) ? std::nullopt : std::make_optional(
            coords);
}

// Exactly the same as the other one, but returns an optional:
std::optional<glm::vec3>
opt_relative_pos_coords(const glm::vec3 &pos, const glm::mat4 &pl_mat, const glm::vec2 &pl_dims) {
    const auto coords = relative_pos_coords(pos, pl_mat, glm::vec2{2.f});
    return (coords.x < 0.f || 1.f < coords.x || coords.y < 0.f || 1.f < coords.y) ? std::nullopt : std::make_optional(
            coords);
}

// Opengl 4.0 Specs: glReadPixels: Pixels are returned in row order from the lowest to the highest row, left to right in each row.
// (0,0) is therefore the top left coordinate (meaning uv coordinates have the v coordinate flipped)
std::optional<glm::vec4>
get_pixel(const glm::uvec2 &coord, const glm::uvec2 tex_dims, const std::vector<glm::vec4> &tex_data) {
    if (tex_data.empty() || tex_dims.x == 0 || tex_dims.y == 0 || tex_dims.x <= coord.x || tex_dims.y <= coord.y)
        return std::nullopt;
    return std::make_optional(tex_data.at(coord.y * tex_dims.x + coord.x));
};

// Bi-linear interpolation of pixel
glm::vec4 sample_tex(const glm::vec2 &uv, const glm::uvec2 tex_dims, const std::vector<glm::vec4> &tex_data) {
    const auto m_get_pixel = [tex_dims, &tex_data = std::as_const(tex_data)](const glm::uvec2 &coord) {
        return get_pixel(coord, tex_dims, tex_data);
    };

    const glm::vec2 pixel_coord = uv * glm::vec2{tex_dims + 1u};
    const glm::vec2 f_pixel_coord = glm::fract(pixel_coord);
    const glm::uvec2 i_pixel_coord = glm::uvec2{pixel_coord - f_pixel_coord};

    const auto aa = m_get_pixel(i_pixel_coord);
    const auto ba = m_get_pixel(glm::uvec2{i_pixel_coord.x + 1u, i_pixel_coord.y});
    const auto ab = m_get_pixel(glm::uvec2{i_pixel_coord.x, i_pixel_coord.y + 1u});
    const auto bb = m_get_pixel(glm::uvec2{i_pixel_coord.x + 1u, i_pixel_coord.y + 1u});
    if (!aa || !ba || !ab || !bb)
        return glm::vec4{0.};

    return glm::mix(
            glm::mix(*aa, *ba, f_pixel_coord.x),
            glm::mix(*ab, *bb, f_pixel_coord.x),
            f_pixel_coord.y);
}

glm::vec4 sphere_sample_tex(const glm::vec3 &pos, float sphere_radius, const glm::uvec2 &tex_dims,
                            const std::vector<glm::vec4> &tex_data) {
    static const glm::mat4 pl_mat{1.0}; // Currently just hardcoding a xy-plane lying in origo
    constexpr float angle_piece = std::numbers::pi_v<float> / 8.f; // How is this not a thing before C++20 ???

    glm::vec4 sum = sample_tex(pos_uvs_in_plane(pos, pl_mat, glm::vec2{2.f}), tex_dims, tex_data);

    for (int i = 0; i < 8; ++i) {
        const auto angle = angle_piece * static_cast<float>(i);
        const auto displacement = glm::vec3{std::sin(angle), std::cos(angle), 0.f} * sphere_radius;
        const auto coord = opt_pos_uvs_in_plane(pos + displacement, pl_mat, glm::vec2{2.f});
        if (coord)
            sum += sample_tex(*coord, tex_dims, tex_data);
    }
    return sum * (1.f / 9.f);
}

std::pair<glm::uvec2, std::vector<glm::vec4>>
generateMipmap(const glm::uvec2 &tex_dims, const std::vector<glm::vec4> &tex_data, glm::uint level = 0) {
    auto[dim, data] = std::pair<glm::uvec2, std::vector<glm::vec4>>{tex_dims, tex_data};
    for (glm::uint i = 0; i < level; ++i) {
        // coord.y * tex_dims.x + coord.x
        for (glm::uint y = 0; y < dim.y / 2; ++y) {
            for (glm::uint x = 0; x < (dim.x / 2); ++x) {
                const auto sum = data.at(2 * y * dim.x + x * 2) + data.at((2 * y + 1) * dim.x + x * 2) +
                                 data.at(2 * y * dim.x + x) + data.at((2 * y + 1) * dim.x + x * 2 + 1);
                data.at(y * dim.x + x) = sum * 0.25f;
            }
        }
        dim /= 2;
    }

    return std::make_pair(dim, std::vector<glm::vec4>{data.begin(), data.begin() + dim.x * dim.y});
}

std::array<std::pair<glm::uvec2, std::vector<glm::vec4>>, 4>
HapticInteractor::generateMipmaps(const glm::uvec2 &tex_dims, const std::vector<glm::vec4> &tex_data) {
    std::array<std::pair<glm::uvec2, std::vector<glm::vec4>>, 4> levels;
    levels.at(0) = std::make_pair(tex_dims, tex_data);
    for (glm::uint i = 1; i < 4; ++i)
        levels.at(i) = generateMipmap(levels.at(i - 1).first, levels.at(i - 1).second, 1);
    return levels;
}

auto
sample_force(const glm::vec3 &pos, const std::array<std::pair<glm::uvec2, std::vector<glm::vec4>>, 4> &tex_mip_maps,
             glm::uint mip_map_level = 0, float surface_softness = 0.f) {
    const glm::mat4 pl_mat{1.0}; // Currently just hardcoding a xy-plane lying in origo

    const auto coords = opt_relative_pos_coords(pos, pl_mat, glm::vec2{2.f});
    // Can't calculate a force outside the bounds of the plane, so return no force
    if (!coords)
        return glm::vec3{0.f};

    // 4 mipmap levels
//    const auto inverse_haptic_amount = std::clamp(coords.z, 0.f, 0.5f) * 2.f;
//    const auto mip_map_level = std::min(static_cast<uint>(std::round(inverse_haptic_amount * 4.f)), 3u);
    const auto&[dims, data] = tex_mip_maps.at(mip_map_level);

    const auto value = sample_tex(glm::vec2{*coords}, dims, data);
    const auto normal = glm::vec3{value} * 2.f - 1.f;
    float dist = coords->z - value.w * 0.01f;
    // If dist is positive, it means we're above the surface = no force applied
    if (0.f < dist)
        return glm::vec3{0.f};

    // Unsure about the coordinate system, so just setting max to 1 for now:
    static constexpr float max_dist = 1.f;
    const float softness_interpolation =
            surface_softness < 0.001f ? 1.f : std::min(dist / (-max_dist * surface_softness), 1.f);

    // Residual force from the surface:
    const auto f_len = glm::length(normal);
    return (f_len < 0.001f ? glm::vec3{0., 0.f, 1.f} : normal * (1.f / f_len)) * softness_interpolation;
}

auto sphere_sample_force(float sphere_radius, const glm::vec3 &pos,
                         const std::array<std::pair<glm::uvec2, std::vector<glm::vec4>>, 4> &tex_mip_maps,
                         glm::uint mip_map_level = 0, float surface_softness = 0.f) {
    constexpr float angle_piece = std::numbers::pi_v<float> / 8.f; // How is this not a thing before C++20 ???

    glm::vec3 sum = sample_force(pos - glm::vec3{0.f, 0.f, sphere_radius}, tex_mip_maps, mip_map_level,
                                 surface_softness);

    for (int i = 0; i < 8; ++i) {
        const auto angle = angle_piece * static_cast<float>(i);
        const auto displacement = glm::vec3{std::sin(angle), std::cos(angle), 0.f} * sphere_radius;
        sum += sample_force(pos + displacement, tex_mip_maps, mip_map_level, surface_softness);
    }
    return sum * (1.f / 9.f);
}

auto get_friction(double friction_scale, glm::dvec3 velocity) {
    const auto v_len = glm::length(velocity);
    if (v_len < 0.01)
        return glm::dvec3{0.0};

    // Uniform friction is applied in opposite direction of movement
    return -velocity * friction_scale;
}


#if defined(DHD) || defined(FAKE_HAPTIC)

void haptic_loop(const std::stop_token &simulation_should_end, HapticInteractor::HapticParams &haptic_params,
                 std::promise<bool> &&setup_results,
                 ReaderChannel<std::array<std::pair<glm::uvec2, std::vector<glm::vec4>>, 4>> &&normal_tex_channel,
                 std::atomic<glm::mat4> &m_view_mat) {
    auto&[global_pos, global_force, interaction_bounds, max_force, surface_softness, sphere_kernel_radius, friction_scale, enable_force,
    sphere_kernel_enabled, enable_friction, mip_map_level] = haptic_params;
    glm::dvec3 local_pos{0.0};
    bool force_enabled = false;
    double max_bound = 0.01;
    std::array<std::pair<glm::uvec2, std::vector<glm::vec4>>, 4> normal_tex_mip_maps;
    constexpr double EPSILON = 0.001;
    auto last_t = chr::steady_clock::now();

#ifdef DHD
    auto last_time = dhdGetTime();

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

    for (; !simulation_should_end.stop_requested(); /*std::this_thread::sleep_for(std::chrono::microseconds{1})*/) {
        // Query for position (actual rate of querying from hardware is controlled by underlying SDK)
        {
            PROFILE("Haptic - Fetch position");
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
        }

        glm::vec3 pos;
        {
            PROFILE("Haptic - Transform space");
            // Transform to scene space:
            // Novint Falcon is in 10x10x10cm space = 0.1x0.1x0.1m = [-0.05, 0.05] m^3'
            // right = y, up = z, forward = -x
            const auto scale_mult =
                    interaction_bounds.load(std::memory_order_relaxed) / max_bound; // [0, max_bound] -> [0, 10]
            pos = local_pos * scale_mult;
        }

        {
            PROFILE("Haptic - Global pos");
            global_pos.store(pos);
        }

        {
            PROFILE("Haptic - Fetch normal tex");
            if (normal_tex_channel.has_update())
                normal_tex_mip_maps = normal_tex_channel.get();
        }

        // Simulation stuff

        glm::dvec3 force{0.0};
        {
            PROFILE("Haptic - Sample force");
            const auto f_max = max_force.load();
            force = glm::dvec3{(sphere_kernel_enabled.load()
                                ? sphere_sample_force(sphere_kernel_radius.load(), pos, normal_tex_mip_maps,
                                                      mip_map_level.load(), surface_softness.load())
                                : sample_force(pos, normal_tex_mip_maps, mip_map_level.load(), surface_softness.load()))};

            if (enable_friction.load() && 0.001 < glm::length(force)) {
                // TODO: Estimate velocity for FAKE_DHD
                glm::dvec3 estimated_velocity;
                dhdGetLinearVelocity(&estimated_velocity.x, &estimated_velocity.y, &estimated_velocity.z);
                force += get_friction(static_cast<double>(friction_scale.load()), estimated_velocity);
            }

            // Clamp velocity to 1
            const auto f_len = glm::length(force);
            if (1.0 < f_len)
                force *= 1.0 / f_len;

            force *= f_max;
        }

        {
            PROFILE("Haptic - Globel force");
            global_force.store(force);
        }

#if defined(DHD) && !defined(NDEBUG)
        if (dhdGetTime() - last_time > 3.0) {
            last_time = dhdGetTime();
            std::cout << std::format("Haptic frequency: {:.3f} KHz, current force: {:.3f} N", dhdGetComFreq(),
                                     glm::length(force)) << std::endl;
        }
#endif

        if (force_enabled) {
            PROFILE("Haptic - Disable force");
            if (!enable_force.load()) {
#ifdef DHD
                dhdEnableForce(DHD_OFF);
#endif
                force_enabled = false;
                std::cout << "Force disabled!" << std::endl;
                continue;
            }
        } else {
            PROFILE("Haptic - Enable force");
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
        {
            PROFILE("Haptic - Set force");
            dhdSetForce(force.x, force.y, force.z);
        }
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
                                   ReaderChannel<std::array<std::pair<glm::uvec2, std::vector<glm::vec4>>, 4>> &&normal_tex_channel)
        : Interactor(viewer) {
#ifdef DHD
    std::cout << std::format("Running dhd SDK version {}", dhdGetSDKVersionStr()) << std::endl;

    /// Possible handling of grip/rotation and other additional Haptic stuff here

#endif

#if defined(DHD) || defined(FAKE_HAPTIC)
    std::promise<bool> setup_results{};
    auto haptics_enabled = setup_results.get_future();
    m_thread = std::jthread{haptic_loop, std::ref(m_params), std::move(setup_results),
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
        m_params.enable_force.store(!m_params.enable_force.load());
#ifndef NDEBUG
    else if (key == GLFW_KEY_P && action == GLFW_PRESS)
        Profiler::reset_profiler();
#endif

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
        auto interaction_bounds = m_params.interaction_bounds.load();
        auto mip_map_level = static_cast<int>(m_mip_map_ui_level);
        auto enable_force = m_params.enable_force.load();
        auto max_force = m_params.max_force.load();
        auto softness = m_params.surface_softness.load();
        int kernel_type = m_params.sphere_kernel.load() ? 1 : 0;
        m_ui_sphere_kernel_size = m_params.sphere_kernel_radius.load();
        bool enable_friction = m_params.friction.load();
        float friction_scale = m_params.friction_scale.load();

        if (ImGui::SliderFloat("Interaction bounds", &interaction_bounds, 0.1f, 10.f))
            m_params.interaction_bounds.store(interaction_bounds);
        if (ImGui::SliderInt("Mip map levels", &mip_map_level, 0, 3)) {
            m_mip_map_ui_level = static_cast<unsigned int>(mip_map_level);
            m_params.mip_map_level.store(m_mip_map_ui_level);
            viewer()->BROADCAST(&HapticInteractor::m_mip_map_ui_level);
        }
        if (ImGui::Checkbox("Enable force (F)", &enable_force))
            m_params.enable_force.store(enable_force);
        if (enable_force) {
            if (ImGui::SliderFloat("Max haptic force (Newtons)", &max_force, 0.f, 9.f))
                m_params.max_force.store(max_force);
            if (ImGui::SliderFloat("Soft surface-ness", &softness, 0.f, 1.f))
                m_params.surface_softness.store(softness);
            if (ImGui::Combo("Kernel type", &kernel_type, "Point\0Sphere"))
                m_params.sphere_kernel.store(kernel_type == 1);
            if (kernel_type == 1 && ImGui::SliderFloat("Kernel radius", &m_ui_sphere_kernel_size, 0.001f, 0.1f)) {
                m_params.sphere_kernel_radius.store(m_ui_sphere_kernel_size);
                viewer()->BROADCAST(&HapticInteractor::m_ui_sphere_kernel_size);
            }
            if (ImGui::Checkbox("Enable uniform friction", &enable_friction))
                m_params.friction.store(enable_friction);
            if (enable_friction && ImGui::SliderFloat("Friction scale", &friction_scale, 0.f, 10.f))
                m_params.friction_scale.store(friction_scale);
        }

        ImGui::EndMenu();
    }


    m_haptic_global_pos = m_params.finger_pos.load();
    m_haptic_global_force = m_params.force.load();
    viewer()->BROADCAST(&HapticInteractor::m_haptic_global_pos);
    viewer()->BROADCAST(&HapticInteractor::m_haptic_global_force);

    m_view_mat.store(glm::inverse(viewer()->viewTransform()));

    // Event such that haptic renderer can be disabled on runtime. For instance if the device loses connection
    static auto last_haptic_enabled = m_haptic_enabled;
    if (m_on_haptic_toggle && last_haptic_enabled != m_haptic_enabled) {
        m_on_haptic_toggle(m_haptic_enabled);
        last_haptic_enabled = m_haptic_enabled;
    }
}