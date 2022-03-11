#include "Physics.h"

#include <numbers>
#include <iostream>
#include <format>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>

using namespace molumes;

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
    const auto coords = opt_pos_uvs_in_plane(pos, pl_mat, glm::vec2{2.f});
    return coords ? std::make_optional(
            glm::vec3{*coords, point_to_plane(pos, glm::vec3{pl_mat[2]}, glm::vec3{pl_mat[3]})}) : std::nullopt;
}

// Opengl 4.0 Specs: glReadPixels: Pixels are returned in row order from the lowest to the highest row, left to right in each row.
// (0,0) is therefore the top left coordinate (meaning uv coordinates have the v coordinate flipped)
std::optional<glm::vec4>
get_pixel(const glm::uvec2 &coord, const glm::uvec2 tex_dims, const std::vector<glm::vec4> &tex_data) {
    if (tex_data.empty() || tex_dims.x == 0 || tex_dims.y == 0 || tex_dims.x <= coord.x || tex_dims.y <= coord.y)
        return std::nullopt;
    return std::make_optional(tex_data.at(coord.y * tex_dims.x + coord.x));
};

// Uses the quaternion between two vectors to spherically interpolate between them. (quite more expensive than lerp)
template<typename T>
glm::vec<3, T, glm::defaultp>
slerp(const glm::vec<3, T, glm::defaultp> &a, const glm::vec<3, T, glm::defaultp> &b, T t) {
    const auto q = glm::angleAxis(t * std::acos(glm::dot(a, b)), glm::normalize(glm::cross(a, b)));
    return q * a;
}

// Bi-linear interpolation of pixel
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

    glm::vec4 s = glm::mix(
            glm::mix(n_aa, n_ba, f_pixel_coord.x),
            glm::mix(n_ab, n_bb, f_pixel_coord.x),
            f_pixel_coord.y);

    return glm::any(glm::isnan(s)) ? glm::vec4{0.f} : s;
}

glm::dvec3 soften_surface_normal(const glm::dvec3 &normal_force, float height, float surface_softness = 0.f) {
    // Unsure about the coordinate system, so just setting max to 1 for now:
    static constexpr float max_dist = 1.f;
    const float softness_interpolation =
            surface_softness < 0.001f ? 1.f : smoothstep(0.f, 1.f,
                                                         std::min(height / (-max_dist * surface_softness), 1.f));

    return normal_force * static_cast<double>(softness_interpolation);
}

/**
 * Samples the normal force, meaning the force pushing away from surface. In our case, this is the constraint force
 * of the haptic device.
 * @return A dvec3 containing normal force, or std::nullopt if there's no force
 */
std::pair<float, std::optional<glm::dvec3>>
sample_normal_force(const glm::vec3 &relative_coords, const glm::uvec2 &tex_dims,
                    const std::vector<glm::vec4> &tex_data, glm::uint mip_map_level = 0,
                    float surface_height_multiplier = 1.f, bool normal_offset = false) {
    const auto value = sample_tex(glm::vec2{relative_coords}, tex_dims, tex_data);
    float height = relative_coords.z - value.w * surface_height_multiplier;
    // If dist is positive, it means we're above the surface = no force applied
    if (0.f < height)
        return {height, std::nullopt};

    // Surface normal
    // At this point the normal has been scaled with the kde value of the surface
    glm::dvec3 normal{value};

    if (normal_offset) {
        /**
         * Sample the normal at an offset from the height, using the sampled normal as an offset basis. The idea is that
         * the offset surface point is (hopefully) closer to the projected point.
         */
        const auto offset = 2.f * glm::vec2{value} / glm::vec2{tex_dims};
        const auto offset_value = sample_tex(glm::vec2{relative_coords} - offset, tex_dims, tex_data);
        normal = {offset_value};
    }

    /**
     * Modulate surface normal with surface scale:
     * Surface normal is uniformly scaled in the plane axis, so the scaling acts as a regular scaling model matrix.
     * But since these are normal vectors along the surface, they should be treated the same way as a
     * "computer graphics normal vector", meaning they should be multiplied with the "normal matrix", the transpose
     * inverse of the model matrix:
     * N = (M^-1)^T => (({[1, 0, 0], [0, 1, 0], [0, 0, S]})^-1)^T
     *  => [x, y, z] -> [x, y, z / S]
     */
    normal.z /= static_cast<double>(surface_height_multiplier);

    // Normalize:
    const auto norm = glm::length(normal);
    normal = norm < 0.0001 ? glm::dvec3{0., 0.f, 1.0} : normal * (1.0 / norm);

    if (glm::any(glm::isnan(normal)))
        return {height, std::nullopt};


    // Find distance to surface (not the same as height, as that is projected down):
//    const auto dist = -height * normal.z; // dist = dot(vec3{0., 0., 1.}, normal) * -height;

    return {height, std::make_optional(normal)};
}

// Projects a point in world space onto the surface specified by a normal_force vector and
glm::dvec3 project_to_surface(const glm::dvec3 &world_pos, const glm::dvec3 &normal_force, float height) {
    // Find distance to surface (not the same as height, as that is projected down):
    const auto dist = -height * normal_force.z; // dist = dot(vec3{0., 0., 1.}, normal) * -height;
    return world_pos + normal_force * dist;
}

Physics::SimulationStepData &Physics::create_simulation_record(const glm::dvec3 &pos) {
    using namespace std::chrono;

    SimulationStepData &current_simulation_step = m_simulation_steps.emplace();
    current_simulation_step.pos = pos;

    auto current_tp = high_resolution_clock::now();
    const auto delta_time = duration_cast<microseconds>(current_tp - m_tp).count();
    current_simulation_step.delta_us = delta_time;
    const auto inv_delta_s = 1000000.0 / (static_cast<double>(delta_time));
    m_tp = current_tp;

    // Estimate velocity for next frame:
    // Same as (local_pos - previous_step.pos) / (delta_time / 1000000.0):
    const auto delta_v = (pos - m_simulation_steps.get_from_back<1>().pos) * inv_delta_s;
    current_simulation_step.velocity = delta_v;
    return current_simulation_step;
}

std::optional<glm::dvec3>
calc_friction(double friction_scale, const glm::dvec3 &user_force, const glm::dvec3 &normal_force) {
    // F_fr = F_fr_dir * F_fr_len = (-||F_u + F_n||) * (|F_n| * (if |v| < threshold then u_s else u_k)), where u_k < u_s
    const auto fr_len = friction_scale * glm::length(normal_force);
    if (fr_len < 0.0001)
        return std::nullopt;

    // f_n + f_fr = -f => f_n + f = -f_fr => -(f_n + f) = f_fr -> f_fr_dir = -||f_n + f||
    const auto f_fr_dir = -glm::normalize(user_force + normal_force);
    return {f_fr_dir * fr_len};
}

auto calc_uniform_friction(SizedQueue<Physics::SimulationStepData, 2> &simulation_steps, const glm::dvec3 &surface_pos,
                           double friction_scale, const glm::dvec3 &normal_force) {
    auto &current_step = simulation_steps.get_from_back<0>();
    auto &last_step = simulation_steps.get_from_back<1>();
    current_step.sticktion_point = last_step.sticktion_point;
    current_step.surface_pos = surface_pos;

    const auto inv_delta_s = 1000000.0 / (static_cast<double>(current_step.delta_us));
    current_step.surface_velocity = (surface_pos - last_step.surface_pos) * inv_delta_s;

    const bool entered_surface = 0.f < last_step.surface_height && current_step.surface_height <= 0.f;
    if (entered_surface) {
        // Place a "sticktion" point onto the spot where we hit the surface (the projected point)
        current_step.sticktion_point = surface_pos;
    }

    // Temporarily just hard coding the kinetic friction scale. It is required that u_k < u_s, but they can both be
    // arbitrary set to anything that fulfills this requirement.
    const auto[u_s, u_k] = std::make_pair(friction_scale, friction_scale * 0.5);
    const auto f_s = current_step.sticktion_point - surface_pos;
    const auto n_len = glm::length(normal_force);

    // Since I'm only using velocity as a directional / guiding vector it shouldn't matter whether it's normalized or not
    const auto velocity = (current_step.surface_velocity + last_step.surface_velocity) * 0.5;
    const auto v_len = glm::length(velocity);

    const auto f_s_len = glm::length(f_s);
    /**
     * The more force exerted onto the surface, giving a higher normal force, the higher the threshold is for when the
     * static friction should switch to dynamic friction.
     */
    const auto static_distance = n_len * u_s;
    // If the velocity is above the threshold, we're using kinetic friction. We then want the sticktion point to keep
    // "hanging" behind our surface point such that we keep maintaining the dynamic friction until we loose momentum
    if (0.0001 < v_len && static_distance < v_len) {
        const auto f_s_dir = velocity * (-1.0 / v_len);
        const auto kinetic_distance = n_len * u_k;
        current_step.sticktion_point = surface_pos + f_s_dir * kinetic_distance;
        return f_s * kinetic_distance;
    } else {
        return f_s * static_distance;
    }
}

struct NormalLevelResult {
    glm::dvec3 normal, soft_normal;
    float height;
};

std::optional<NormalLevelResult>
sample_normal_level(const TextureMipMaps &tex_mip_maps, SizedQueue<Physics::SimulationStepData, 2> &simulation_steps,
                    const glm::dvec3 &coords, unsigned int level,
                    float surface_height_multiplier, bool normal_offset, float surface_force, float surface_softness) {
    const auto&[dims, data] = tex_mip_maps.at(level);

    // 3-4. Calculate normal (constraint) force
    const auto[surface_height, opt_normal_force] = sample_normal_force(coords, dims, data, level,
                                                                       surface_height_multiplier, normal_offset);

    simulation_steps.get_from_back<0>().surface_height = surface_height;

    // Early exit of there's no surface force (we're moving through air / empty space)
    if (!opt_normal_force)
        return std::nullopt;

    // Scale normal force with normal_force:
    auto normal_force = *opt_normal_force * static_cast<double>(surface_force);

    // Save the unmodified normal:
    simulation_steps.get_from_back<0>().normal_force = *opt_normal_force;
    // Adjust normal by using the half of the last normal_vector (smoothes out big changes in normal, like a moving over a bump)
    auto half_normal = simulation_steps.get_from_back<1>().normal_force + normal_force;
    const auto half_normal_len = glm::length(half_normal);
    if (0.001 < half_normal_len) {
        half_normal *= (static_cast<double>(surface_force) / half_normal_len);
        normal_force = half_normal;
    }

    const auto soft_normal_force = soften_surface_normal(normal_force, surface_height, surface_softness);
    return std::make_optional(
            NormalLevelResult{.normal = normal_force, .soft_normal = soft_normal_force, .height = surface_height});
}

namespace molumes {
    glm::dvec3 Physics::simulate_and_sample_force(float surface_force, float surface_softness, float friction_scale,
                                                  float surface_height_multiplier, FrictionMode friction_mode,
                                                  unsigned int mip_map_level, const TextureMipMaps &tex_mip_maps,
                                                  const glm::dvec3 &pos, bool normal_offset,
                                                  std::optional<float> gravity_factor, bool softLODsMode) {
        auto &current_simulation_step = create_simulation_record(pos);
        const glm::mat4 pl_mat{1.0}; // Currently just hardcoding a xy-plane lying in origo
        glm::dvec3 sum_forces{0.0};

        // Optional gravity force (always constant, does not care whether inside bounds or not)
        if (gravity_factor)
            sum_forces.z -= *gravity_factor;

        const auto coords = opt_relative_pos_coords(glm::vec3{pos}, pl_mat, glm::vec2{2.f});
        // Can't calculate a force outside the bounds of the plane, so return no force
        if (!coords)
            return sum_forces;

        // Shouldn't be possible for coords to be negative
        assert(!glm::any(glm::lessThan(glm::vec2{*coords}, glm::vec2{0.f})));

        glm::dvec3 sum_normal_force{0.0};
        std::optional<NormalLevelResult> sample_level_results;
        double normal_force_multiplier{1.0};
        if (softLODsMode) {
            // Get upper and lower mip map levels:
            const auto f_i = std::clamp(coords->z + 0.5f, 0.f, 1.f) * (HapticMipMapLevels - 1);
            const auto upper_level = static_cast<int>(std::ceil(f_i));
            const auto lower_level = static_cast<int>(std::floor(f_i));

            normal_force_multiplier = static_cast<float>(lower_level) / static_cast<float>(HapticMipMapLevels - 1);
            sample_level_results = sample_normal_level(tex_mip_maps, m_simulation_steps, *coords,
                                                                  lower_level, surface_height_multiplier,
                                                                  normal_offset, surface_force, surface_softness);
            /*
             * If lower level normal doesn't exist, it means we're above the lower surface. So use the normal force
             * from the upper level instead.
             */
            if (!sample_level_results) {
                sample_level_results = sample_normal_level(tex_mip_maps, m_simulation_steps, *coords,
                                                           upper_level, surface_height_multiplier,
                                                           normal_offset, surface_force, surface_softness);
                normal_force_multiplier = static_cast<float>(upper_level) / static_cast<float>(HapticMipMapLevels - 1);
            }

            // If it also misses the upper level, we're above all the surfaces (in air). Return early...
        } else {
            sample_level_results = sample_normal_level(tex_mip_maps, m_simulation_steps, *coords,
                                                                  mip_map_level, surface_height_multiplier,
                                                                  normal_offset, surface_force, surface_softness);
        }
        // We're above the (all) surface(s). In the air, return early.
        if (!sample_level_results)
            return sum_forces;

        auto [normal_force, soft_normal_force, surface_height] = *sample_level_results;
        if (softLODsMode) {

        }

        // Note: Currently only uniform friction is implemented
        if (friction_mode == FrictionMode::Uniform) {
            const auto surface_pos = project_to_surface(pos, normal_force, surface_height);
            const auto friction = calc_uniform_friction(m_simulation_steps, surface_pos,
                                                        static_cast<double>(friction_scale),
                                                        soft_normal_force);
            sum_forces += friction;
        }

        sum_forces += sum_normal_force;

        return sum_forces;
    }
}