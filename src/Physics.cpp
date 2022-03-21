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

template<typename T, typename R>
std::optional<R> optional_chain(const std::optional<T> &opt) {
    return opt ? std::make_optional<R>(*opt) : std::nullopt;
}

template<typename T, typename F>
auto optional_chain(const std::optional<T> &opt, F &&func) {
    return opt ? func(*opt) : std::nullopt;
}

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
    return optional_chain(coords, [&](const auto &coord) {
        return std::make_optional<glm::vec3>(coord, point_to_plane(pos, glm::vec3{pl_mat[2]}, glm::vec3{pl_mat[3]}));
    });
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
//    if (0.f < height)
//        return {height, std::nullopt};

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

// Fibonacci sphere algorithm, inspired by https://stackoverflow.com/questions/9600801/evenly-distributing-n-points-on-a-sphere
std::vector<glm::dvec3> fibonacci_sphere(unsigned int count = 8, double radius = 1.0) {
    // golden angle in radians
    static const auto phi = std::numbers::pi * (3.0 - std::sqrt(5.0));
    std::vector<glm::dvec3> points;
    points.reserve(count);

    for (auto i{0u}; i < count; ++i) {
        const auto y = radius - (static_cast<double>(i) / static_cast<double>(count - 1)) * 2.0 * radius;
        const auto y_radius = std::sqrt(radius - y * y);
        const auto theta = phi * static_cast<double>(i);

        points.emplace_back(std::cos(theta) * y_radius, std::sin(theta) * y_radius, y);
    }

    return points;
}

// Does a sphere sample around the center and chooses a new point that is closer to the surface within a specified radius
glm::dvec3 find_closest_point_in_sphere(const glm::dvec3 &center, double radius, const TextureMipMaps &tex_mip_maps,
                                        unsigned int level, float surface_height_multiplier) {
    auto sample_points = fibonacci_sphere(18, radius);
    for (auto &p: sample_points)
        p += center;
    sample_points.push_back(center);
    std::pair<double, glm::dvec3> closest{std::numeric_limits<double>::max(), {}};

    const auto&[dims, data] = tex_mip_maps.at(level);
    for (const auto &p: sample_points) {
        if (p.x < 0.0 || 1.0 < p.x || p.y < 0.0 || 1.0 < p.y)
            continue;
        const auto value = sample_tex(glm::vec2{p}, dims, data);
        auto dist = std::abs(p.z - value.w * surface_height_multiplier);
        if (dist < closest.first)
            closest = {dist, p};
    }

    // Since we include the center, we should be guaranteed that at least 1 point will be closest
    return closest.second;
}

/**
 * @brief Samples texture slices like a volume.
 * Using 2 slices, samples a density in a volume constructed from the slices, using the (un?)signed height differences
 * as the density. The xy-part of the sampling coordinates translates to the uv-coordinates in the plane, and the
 * z-coordinate translates to the difference between the 2 slices - the interpolation from the first layer to the
 * second layer.
 */
float sample_volume_tf(const glm::vec3 &coord, const TextureMipMaps &tex_mip_maps,
                       std::array<unsigned int, 2> levels) {
    const auto&[dims0, data0] = tex_mip_maps.at(levels[0]);
    const auto&[dims1, data1] = tex_mip_maps.at(levels[1]);

    // If coordinates are outside of bounds, just return 0 as the density (air)
    if (glm::any(glm::lessThan(coord, glm::vec3{0.f})) || glm::any(glm::greaterThan(coord, glm::vec3{1.f})))
        return 0.0;

    return std::lerp(
            sample_tex(glm::vec2{coord}, dims0, data0).w,
            sample_tex(glm::vec2{coord}, dims1, data1).w,
            coord.z);
}

// Sample a volume gradient using central differences
glm::dvec3 sample_volume_gradient(const TextureMipMaps &tex_mip_maps, const glm::vec3 &coords,
                                  std::array<unsigned int, 2> levels, float interp = 0.5f) {
    static constexpr double EPS = 0.01f;
    using namespace glm;
    const auto &tf = [&tex_mip_maps, levels](const vec3 &coord) {
        return sample_volume_tf(coord, tex_mip_maps, levels);
    };

    return {
            tf(vec3{coords.x + EPS, coords.y, interp}) - tf(vec3{coords.x - EPS, coords.y, interp}),
            tf(vec3{coords.x, coords.y + EPS, interp}) - tf(vec3{coords.x, coords.y - EPS, interp}),
            tf(vec3{coords.x, coords.y, interp + EPS}) - tf(vec3{coords.x, coords.y, interp - EPS}),
    };
}

struct NormalLevelSampleResult {
    glm::dvec3 normal;
    float height;
};

struct NormalLevelResult {
    glm::dvec3 normal, soft_normal;
    float height;
};

std::optional<NormalLevelSampleResult>
sample_normal_level(const TextureMipMaps &tex_mip_maps, SizedQueue<Physics::SimulationStepData, 2> &simulation_steps,
                    glm::dvec3 coords, unsigned int level,
                    float surface_height_multiplier, bool normal_offset, float surface_force, float surface_softness) {
    const auto&[dims, data] = tex_mip_maps.at(level);

//    // Find a better estimation to closest point:
//    coords = find_closest_point_in_sphere(coords, 0.00001, tex_mip_maps, level,
//                                          surface_height_multiplier);


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

    return {{normal_force, surface_height}};
}

namespace molumes {
    glm::dvec3 Physics::simulate_and_sample_force(float surface_force, float surface_softness, float friction_scale,
                                                  float surface_height_multiplier, FrictionMode friction_mode,
                                                  unsigned int mip_map_level, const TextureMipMaps &tex_mip_maps,
                                                  const glm::dvec3 &pos, bool normal_offset,
                                                  std::optional<float> gravity_factor,
                                                  std::optional<unsigned int> surface_volume_mip_map_counts,
                                                  std::optional<float> sphere_kernel_radius,
                                                  bool linear_volume_surface_force) {
        auto &current_simulation_step = create_simulation_record(pos);
        const glm::mat4 pl_mat{1.0}; // Currently just hardcoding a xy-plane lying in origo
        glm::dvec3 sum_forces{0.0};

        // Optional gravity force (always constant, does not care whether inside bounds or not)
        if (gravity_factor)
            sum_forces.z -= *gravity_factor;

        const auto coords = opt_relative_pos_coords(glm::vec3{pos}, pl_mat, glm::vec2{2.f});
        if (!coords) {
            // Apply a constant up-facing surface force outside of bounds (simulating an infinite plane)
            sum_forces += soften_surface_normal(glm::dvec3{0.f, 0.f, surface_force},
                                                point_to_plane(pos, glm::vec3{pl_mat[2]}, glm::vec3{pl_mat[3]}),
                                                surface_softness);
            return sum_forces;
        }

        // Shouldn't be possible for coords to be negative
        assert(!glm::any(glm::lessThan(glm::vec2{*coords}, glm::vec2{0.f})));

        glm::dvec3 sum_normal_force{0.0};
        std::optional<NormalLevelResult> sample_level_results;
        // Surface volume (multiple surfaces)
        if (surface_volume_mip_map_counts) {
            const auto enabled_mip_maps = generate_enabled_mip_maps(*surface_volume_mip_map_counts);
            const auto surface_height = [](float height_interp, int lower, int upper) {
                return 0.5f * std::lerp(static_cast<float>(lower), static_cast<float>(upper), height_interp) /
                       (HapticMipMapLevels - 1) - 0.25f;
            };

            const auto level_relative_height =
                    [enabled_levels_count = static_cast<float>(enabled_mip_maps.size() - 1)]
                            (auto level_index) {
                        return std::lerp(-0.25f, 0.25f, static_cast<float>(level_index) / enabled_levels_count);
                    };
            const auto level_relative_coordinates = [level_relative_height](const glm::vec3 &coords, auto level_index) {
                // 2x+0.5 = t <=> t/2-0.25 = x
                return coords - glm::vec3{0.f, 0.f, level_relative_height(level_index)};
            };

            // Get upper and lower mip map levels:
            const auto enabled_mip_maps_range_mult = static_cast<float>(*surface_volume_mip_map_counts - 1);
            const auto t = std::clamp(coords->z * 2.f + 0.5f, 0.f, 1.f);

            const auto upper_j = static_cast<std::size_t>(std::ceil(t * enabled_mip_maps_range_mult));
            const auto lower_j = static_cast<std::size_t>(std::floor(t * enabled_mip_maps_range_mult));
            auto upper_level = static_cast<unsigned int>(enabled_mip_maps.at(upper_j));
            auto lower_level = static_cast<unsigned int>(enabled_mip_maps.at(lower_j));

            const auto f_f = t * enabled_mip_maps_range_mult - static_cast<float>(lower_j);

            const auto gradient = sample_volume_gradient(tex_mip_maps, *coords, {lower_level, upper_level}, f_f) *
                                  static_cast<double>(surface_force) * 100.0;

            if (0.001 < glm::length(gradient)) {
                sample_level_results = {{.normal = gradient, .soft_normal = gradient, .height = coords->z}};
            }

//            auto is_top_level = upper_level == HapticMipMapLevels - 1;
//
//            auto r1 = sample_normal_level(tex_mip_maps, m_simulation_steps,
//                                          level_relative_coordinates(*coords, lower_j),
//                                          lower_level, surface_height_multiplier,
//                                          normal_offset, 1.f, surface_softness);
//            if (lower_level == upper_level) {
//                const auto soft_normal = is_top_level ? soften_surface_normal(r1->normal, r1->height, surface_softness)
//                                                      : r1->normal;
//                sample_level_results =
//                        r1 && r1->height <= 0.f ? std::make_optional<NormalLevelResult>(r1->normal, soft_normal,
//                                                                                        r1->height) : std::nullopt;
//            } else {
//                auto r2 = sample_normal_level(tex_mip_maps, m_simulation_steps,
//                                              level_relative_coordinates(*coords, upper_j), upper_level,
//                                              surface_height_multiplier, normal_offset, 1.f, surface_softness);
//
//                if (r1 && r2) {
//                    // The relative interpolation between the height levels (like f_f but corresponding to heights):
//                    auto h_t = r1->height / (r1->height - r2->height);
//                    // If h_t is negative, our probe is below our lower_bounds sampled height, so shift the interval one down
//                    const auto should_shift_down = h_t < 0.f && lower_j != 0;
//                    if (should_shift_down) {
//                        r2 = r1;
//                        upper_level = lower_level;
//                        lower_level = enabled_mip_maps.at(lower_j - 1);
//
//                        r1 = sample_normal_level(tex_mip_maps, m_simulation_steps,
//                                                 level_relative_coordinates(*coords, lower_j - 1),
//                                                 lower_level, surface_height_multiplier, normal_offset, 1.f,
//                                                 surface_softness);
//                        h_t = r1->height / (r1->height - r2->height);
//                        is_top_level = false; // Since we shifted down, it's guaranteed to not be the top level anymore
//                    }
//
//                    const auto n = glm::mix(r1->normal, is_top_level ? soften_surface_normal(r2->normal, r2->height,
//                                                                                             surface_softness)
//                                                                     : r2->normal, h_t);
//                    sample_level_results = std::make_optional<NormalLevelResult>(n, n, surface_height(h_t, lower_level,
//                                                                                                      upper_level));
//                } else {
//                    sample_level_results = optional_chain(r1 ? r1 : r2, [](const auto &s) {
//                        return std::make_optional<NormalLevelResult>(s.normal, s.normal, s.height);
//                    });
//                }
//            }
//
//            if (sample_level_results) {
//                const auto force_multiplier = std::lerp(surface_force, surface_force * 0.25f,
//                                                        linear_volume_surface_force ? t : (
//                                                                static_cast<float>(lower_level) /
//                                                                (HapticMipMapLevels - 1)));
//
//                sample_level_results->normal *= force_multiplier;
//                sample_level_results->soft_normal *= force_multiplier;
//            }
            // Singular surface:
        } else {
            const auto res = sample_normal_level(tex_mip_maps, m_simulation_steps, *coords,
                                                 mip_map_level, surface_height_multiplier,
                                                 normal_offset, surface_force, surface_softness);
            sample_level_results = optional_chain(res, [=](const auto &r) {
                return std::make_optional<NormalLevelResult>(r.normal, soften_surface_normal(r.normal, r.height,
                                                                                             surface_softness),
                                                             r.height);
            });
        }
        // We're above the (all) surface(s). In the air, return early.
        if (!sample_level_results)
            return sum_forces;

        auto[normal_force, soft_normal_force, surface_height] = *sample_level_results;
        sum_forces += soft_normal_force;

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

    std::vector<int> generate_enabled_mip_maps(unsigned int enabled_count) {
        std::vector<int> out;
        out.reserve(enabled_count);
        for (unsigned int i{0}; i < enabled_count; ++i)
            out.push_back(static_cast<int>(std::round(
                    (HapticMipMapLevels - 1) * static_cast<float>(i) / static_cast<float>(enabled_count - 1) + 0.01f
            )));
        return out;
    }
}