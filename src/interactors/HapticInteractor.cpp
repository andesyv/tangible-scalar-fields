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

// Opengl 4.0 Specs: glReadPixels: Pixels are returned in row order from the lowest to the highest row, left to right in each row.
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
             glm::uint mip_map_level = 0) {
    const glm::mat4 pl_mat{1.0}; // Currently just hardcoding a xy-plane lying in origo

    const auto coords = relative_pos_coords(pos, pl_mat, glm::vec2{2.f});
    // Can't calculate a force outside the bounds of the plane, so return no force
    if (coords.x < 0.f || 1.f < coords.x || coords.y < 0.f || 1.f < coords.y)
        return glm::vec3{0.f};

    // 4 mipmap levels
//    const auto inverse_haptic_amount = std::clamp(coords.z, 0.f, 0.5f) * 2.f;
//    const auto mip_map_level = std::min(static_cast<uint>(std::round(inverse_haptic_amount * 4.f)), 3u);
    const auto&[dims, data] = tex_mip_maps.at(mip_map_level);

    const auto value = sample_tex(glm::vec2{coords}, dims, data);
    const auto normal = glm::vec3{value} * 2.f - 1.f;
    float dist = coords.z - value.w * 0.01f;
    // If dist is positive, it means we're above the surface = no force applied
    if (0.f < dist)
        return glm::vec3{0.f};

    const auto v_len = glm::length(normal);

    // If sampled tex was 0, value can still be zero.
    return v_len < 0.001f ? glm::vec3{0.f} : normal * (1.f / v_len)/* (1.f - inverse_haptic_amount)*/; // Clamp to 1 Newton
}

#if defined(DHD) || defined(FAKE_HAPTIC)

void haptic_loop(std::stop_token simulation_should_end, std::atomic<glm::vec3> &global_pos,
                 std::atomic<float> &interaction_bounds, std::atomic<bool> &enable_force, std::atomic<float> &max_force,
                 std::atomic<glm::uint> &mip_map_level, std::promise<bool> &&setup_results,
                 ReaderChannel<std::array<std::pair<glm::uvec2, std::vector<glm::vec4>>, 4>> &&normal_tex_channel,
                 std::atomic<glm::mat4> &m_view_mat) {
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

#ifdef DHD
        {
            PROFILE("Haptic - Enable force");
            // Simulation stuff
            if (!enable_force.load()) {
                if (force_enabled) {
                    dhdEnableForce(DHD_OFF);
                    force_enabled = false;
                    std::cout << "Force disabled!" << std::endl;
                }
                continue;
            }
        }
#endif

        glm::dvec3 force;
        {
            PROFILE("Haptic - Sample force");
            force = glm::dvec3{sample_force(pos, normal_tex_mip_maps, mip_map_level.load()) * max_force.load()};
        }

//#if defined(DHD) && !defined(NDEBUG)
//        if (dhdGetTime() - last_time > 0.1) {
//            last_time = dhdGetTime();
//            std::cout << std::format("Frequency: {}KHz, force: {}N", dhdGetComFreq(), glm::length(force)) << std::endl;
//        }
//#endif

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
    m_thread = std::jthread{haptic_loop, std::ref(m_haptic_finger_pos), std::ref(m_interaction_bounds),
                            std::ref(m_enable_force), std::ref(m_max_force), std::ref(m_mip_map_level),
                            std::move(setup_results), std::move(normal_tex_channel), std::ref(m_view_mat)};
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
        auto interaction_bounds = m_interaction_bounds.load();
        auto mip_map_level = static_cast<int>(m_mip_map_ui_level);
        auto enable_force = m_enable_force.load();
        auto max_force = m_max_force.load();
        if (ImGui::SliderFloat("Interaction bounds", &interaction_bounds, 0.1f, 10.f))
            m_interaction_bounds.store(interaction_bounds);
        if (ImGui::SliderInt("Mip map levels", &mip_map_level, 0, 3)) {
            m_mip_map_ui_level = static_cast<unsigned int>(mip_map_level);
            m_mip_map_level.store(m_mip_map_ui_level);
            viewer()->broadcast(getTypeHash(&HapticInteractor::m_mip_map_ui_level), m_mip_map_ui_level);
        }
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
