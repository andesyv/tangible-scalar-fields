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

// AMD C++ smoothstep function from 0 to 1 (https://en.wikipedia.org/wiki/Smoothstep)
float smoothstep(float a, float b, float x) {
    // Scale, bias and saturate x to 0..1 range
    x = std::clamp((x - a) / (b - a), 0.f, 1.f);
    // Evaluate polynomial
    return x * x * (3.f - 2.f * x);
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

glm::vec4 scaling_mix(const glm::vec4 &a, const glm::vec4 &b, float t) {
    const auto &c = glm::mix(a, b, t);
    const auto diff = b.w - a.w;
    return glm::vec4{c.x, c.y, 0.001f < std::abs(diff) ? c.z / diff : c.z, c.w};
};

// Bi-linear interpolation of pixel
template<bool SCALE_NORMAL = false>
glm::vec4 sample_tex(const glm::vec2 &uv, const glm::uvec2 tex_dims, const std::vector<glm::vec4> &tex_data) {
    const auto m_get_pixel = [tex_dims, &tex_data = std::as_const(tex_data)](const glm::uvec2 &coord) {
        return get_pixel(coord, tex_dims, tex_data);
    };

    const glm::vec2 pixel_coord = uv * glm::vec2{tex_dims + 1u};
    const glm::vec2 f_pixel_coord = glm::fract(pixel_coord);
    const glm::uvec2 i_pixel_coord = glm::uvec2{pixel_coord - f_pixel_coord};

    auto aa = m_get_pixel(i_pixel_coord);
    auto ba = m_get_pixel(glm::uvec2{i_pixel_coord.x + 1u, i_pixel_coord.y});
    auto ab = m_get_pixel(glm::uvec2{i_pixel_coord.x, i_pixel_coord.y + 1u});
    auto bb = m_get_pixel(glm::uvec2{i_pixel_coord.x + 1u, i_pixel_coord.y + 1u});
    if (!aa || !ba || !ab || !bb)
        return glm::vec4{0.};

    // Convert to normalized range:
    const auto n_aa = glm::vec4{glm::vec3{*aa} * 2.f - 1.f, aa->w};
    const auto n_ba = glm::vec4{glm::vec3{*ba} * 2.f - 1.f, ba->w};
    const auto n_ab = glm::vec4{glm::vec3{*ab} * 2.f - 1.f, ab->w};
    const auto n_bb = glm::vec4{glm::vec3{*bb} * 2.f - 1.f, bb->w};

    glm::vec4 s;
    if constexpr (SCALE_NORMAL) {
        s = scaling_mix(
                scaling_mix(n_aa, n_ba, f_pixel_coord.x),
                scaling_mix(n_ab, n_bb, f_pixel_coord.x),
                f_pixel_coord.y);
    } else {
        s = glm::mix(
                glm::mix(n_aa, n_ba, f_pixel_coord.x),
                glm::mix(n_ab, n_bb, f_pixel_coord.x),
                f_pixel_coord.y);
    }

    return glm::any(glm::isnan(s)) ? glm::vec4{0.f} : s;
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

auto
sample_surface_force(const glm::vec3 &relative_coords, const glm::uvec2 &tex_dims,
                     const std::vector<glm::vec4> &tex_data,
                     glm::uint mip_map_level = 0, float surface_softness = 0.f, float surface_height = 1.f) {
    const auto value = sample_tex<true>(glm::vec2{relative_coords}, tex_dims, tex_data);
    float height = relative_coords.z - value.w * 0.01f * surface_height;
    // If dist is positive, it means we're above the surface = no force applied
    if (0.f < height)
        return glm::vec3{0.f};

    // Unsure about the coordinate system, so just setting max to 1 for now:
    static constexpr float max_dist = 1.f;
    const float softness_interpolation =
            surface_softness < 0.001f ? 1.f : smoothstep(0.f, 1.f,
                                                         std::min(height / (-max_dist * surface_softness), 1.f));

    // Surface normal
    // At this point the normal has been scaled with the kde value of the surface
    auto normal = glm::vec3{value};
    const auto norm = glm::length(normal);
    normal = norm < 0.001f ? glm::vec3{0., 0.f, 1.f} : normal * (1.f / norm);

    /**
     * Modulate surface normal with kde-height:
     * Surface normal is uniformly scaled in the plane axis, so the scaling acts as a regular scaling model matrix.
     * But since these are normal vectors along the surface, they should be treated the same way as a
     * "computer graphics normal vector", meaning they should be multiplied with the "normal matrix", the transpose
     * inverse of the model matrix:
     * N = (M^-1)^T => (({[1, 0, 0], [0, 1, 0], [0, 0, S]})^-1)^T
     *  => [x, y, z] -> [x, y, z / S]
     */
    normal.z /= surface_height;
    normal = glm::normalize(normal);

    if (glm::any(glm::isnan(normal)))
        return glm::vec3{0.f};

    // Find distance to surface (not the same as height, as that is projected down):
    const auto dist = -height * normal.z; // dist = dot(vec3{0., 0., 1.}, normal) * -height;

    // Final force, the amount we want to push outwards (not along the surface) = normal * dist * softness_interpolation
    return normal * dist * softness_interpolation;
}

auto sphere_sample_surface_force(float sphere_radius, const glm::vec3 &pos,
                                 const glm::uvec2 &tex_dims, const std::vector<glm::vec4> &tex_data,
                                 glm::uint mip_map_level = 0, float surface_softness = 0.f) {
    constexpr float angle_piece = std::numbers::pi_v<float> / 8.f; // How is this not a thing before C++20 ???

    const glm::mat4 pl_mat{1.0}; // Currently just hardcoding a xy-plane lying in origo
    const auto coords = relative_pos_coords(pos - glm::vec3{0.f, 0.f, sphere_radius}, pl_mat, glm::vec2{2.f});

    glm::vec3 sum = sample_surface_force(coords, tex_dims, tex_data, mip_map_level,
                                         surface_softness);

    for (int i = 0; i < 8; ++i) {
        const auto angle = angle_piece * static_cast<float>(i);
        const auto displacement = glm::vec3{std::sin(angle), std::cos(angle), 0.f} * sphere_radius;
        const auto coord = opt_relative_pos_coords(pos + displacement, pl_mat, glm::vec2{2.f});
        if (coord)
            sum += sample_surface_force(*coord, tex_dims, tex_data, mip_map_level, surface_softness);
    }
    return sum * (1.f / 9.f);
}

auto get_friction(double friction_scale, glm::dvec3 velocity, glm::dvec3 normal, glm::dvec3 last_normal) {
    const auto v_len = glm::length(velocity);
    if (v_len < 0.01)
        return glm::dvec3{0.0};

    const auto v_normalized = velocity * (-1.f / v_len);

    constexpr glm::dvec3 up = {0.0, 0.0, 1.0};
    const auto normal_grad = normal - last_normal;

    // If the dot product of the gradient and velocity is negative, we're moving away from the top (I think)
    const auto k = glm::dot(normal_grad, velocity);
    return k < 0.0 ? -k * v_normalized * friction_scale : glm::dvec3{0.0};


//    // The steeper the normal, the more aligned with -velocity the normal will be, increasing the dot product
//    // Multiplied with (1.0 - normal.z) to make it stronger the more it's not aligned with the default normal (straight up)
//    const auto projected_velocity = std::max(glm::dot(v_normalized, normal) * (1.0 - normal.z), 0.0);
//
//    return v_normalized * friction_scale * projected_velocity;
}

using TextureMipMaps = std::array<std::pair<glm::uvec2, std::vector<glm::vec4>>, HapticMipMapLevels>;

#if defined(DHD) || defined(FAKE_HAPTIC)

glm::dvec3 sample_force(float max_force, float surface_softness, float sphere_kernel_radius, float friction_scale,
                        float surface_height, bool sphere_kernel_enabled, bool enable_friction, bool uniform_friction,
                        unsigned int mip_map_level,
                        const TextureMipMaps &tex_mip_maps,
                        const glm::vec3 &pos) {
    const glm::mat4 pl_mat{1.0}; // Currently just hardcoding a xy-plane lying in origo

    const auto coords = opt_relative_pos_coords(pos, pl_mat, glm::vec2{2.f});
    // Can't calculate a force outside the bounds of the plane, so return no force
    if (!coords)
        return glm::vec3{0.f};

    const auto&[dims, data] = tex_mip_maps.at(mip_map_level);

    auto force = glm::dvec3{
            (sphere_kernel_enabled ? sphere_sample_surface_force(sphere_kernel_radius, pos, dims, data,
                                                                 mip_map_level, surface_softness)
                                   : sample_surface_force(*coords, dims, data, mip_map_level, surface_softness,
                                                          surface_height))};

    if (enable_friction && 0.001 < glm::length(force)) {
        // TODO: Estimate velocity for FAKE_DHD
        glm::dvec3 estimated_velocity{0.f};
#ifdef DHD
        dhdGetLinearVelocity(&estimated_velocity.x, &estimated_velocity.y, &estimated_velocity.z);
#endif

        const auto&[friction_dims, friction_data] = tex_mip_maps.at(
                uniform_friction ? mip_map_level : std::min<std::size_t>(mip_map_level + 4, HapticMipMapLevels));
        const auto current_normal = glm::normalize(
                glm::dvec3{sample_tex(glm::vec2{*coords}, friction_dims, friction_data)} * 2.0 - 1.0);
        static auto last_normal = current_normal;

        // Should'nt be possible for coords to be negative
        assert(!glm::any(glm::lessThan(glm::vec2{*coords}, glm::vec2{0.f})));

//        if (!glm::any(glm::isnan(current_normal))) {
//            const auto friction = get_friction(static_cast<double>(friction_scale), estimated_velocity, current_normal, last_normal);
//            std::cout << std::format("Friction: {}", glm::length(friction)) << std::endl;
////            force += friction;
//        }

        last_normal = current_normal;
    }

    // Clamp force to 1
    const auto f_len = glm::length(force);
    if (1.0 < f_len)
        force *= 1.0 / f_len;

    force *= max_force;
    return force;
}

void haptic_loop(const std::stop_token &simulation_should_end, HapticInteractor::HapticParams &haptic_params,
                 std::promise<bool> &&setup_results,
                 ReaderChannel<TextureMipMaps> &&normal_tex_channel,
                 std::atomic<glm::mat4> &m_view_mat) {
    auto&[global_pos, global_force, interaction_bounds, max_force, surface_softness, sphere_kernel_radius,
    friction_scale, surface_height, enable_force, sphere_kernel_enabled, enable_friction, uniform_friction,
    mip_map_level] = haptic_params;
    glm::dvec3 local_pos{0.0};
    bool force_enabled = false;
    double max_bound = 0.01;
    TextureMipMaps normal_tex_mip_maps;
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
            force = sample_force(max_force.load(), surface_softness.load(), sphere_kernel_radius.load(),
                                 friction_scale.load(), surface_height.load(), sphere_kernel_enabled.load(),
                                 enable_friction.load(), uniform_friction.load(), mip_map_level.load(),
                                 normal_tex_mip_maps, pos);

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
                                   ReaderChannel<TextureMipMaps> &&normal_tex_channel)
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
        int friction_type = m_params.friction.load() ? (m_params.uniform_friction.load() ? 1 : 2) : 0;
        float friction_scale = m_params.friction_scale.load();
        m_ui_surface_height_multiplier = m_params.surface_height_multiplier.load();

        if (ImGui::SliderFloat("Interaction bounds", &interaction_bounds, 0.1f, 10.f))
            m_params.interaction_bounds.store(interaction_bounds);
        if (ImGui::SliderInt("Mip map level", &mip_map_level, 0, HapticMipMapLevels - 1)) {
            m_mip_map_ui_level = static_cast<unsigned int>(mip_map_level);
            m_params.mip_map_level.store(m_mip_map_ui_level);
            viewer()->BROADCAST(&HapticInteractor::m_mip_map_ui_level);
        }
        if (ImGui::SliderFloat("Surface height multiplier", &m_ui_surface_height_multiplier, 0.1f, 10.f)) {
            m_params.surface_height_multiplier.store(m_ui_surface_height_multiplier);
            viewer()->BROADCAST(&HapticInteractor::m_ui_surface_height_multiplier);
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
            if (ImGui::Combo("Friction", &friction_type, "None\0Uniform\0Directional")) {
                m_params.friction.store(friction_type != 0);
                m_params.uniform_friction.store(friction_type == 1);
            }
            if (friction_type != 0 && ImGui::SliderFloat("Friction scale", &friction_scale, 0.f, 1000.f))
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