#include "Physics.h"
#include "Profile.h"

#include <numbers>
#include <iostream>
#include <format>
#include <numeric>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/random.hpp>

using namespace molumes;

template<glm::length_t L, typename T, glm::qualifier Q>
struct std::formatter<glm::vec<L, T, Q>> : std::formatter<std::string> {
    auto format(const glm::vec<L, T, Q> &v, format_context &ctx) {
        return formatter<string>::format(glm::to_string(v), ctx);
    }
};

template<typename T, typename F>
auto optional_chain(const std::optional<T> &opt, F &&func) {
    return opt ? func(*opt) : std::nullopt;
}

// AMD C++ smoothstep function from 0 to 1 (https://en.wikipedia.org/wiki/Smoothstep)
float smoothstep(float x) {
    x = std::clamp(x, 0.f, 1.f);
    // Evaluate polynomial
    return x * x * (3.f - 2.f * x);
}

// returns signed distance to plane (negative is below)
float point_to_plane(const glm::vec3 &pos, const glm::vec3 &pl_norm, const glm::vec3 &pl_pos) {
    glm::vec3 pl_to_pos = pos - pl_pos;
    return glm::dot(pl_to_pos, pl_norm);
}

// Finds relative uv coords in a plane given the planes tangent and bi-tangent
glm::vec2
pos_uvs_in_plane(const glm::vec3 &pos, const glm::vec3 &pl_tan, const glm::vec3 &pl_bitan, const glm::vec2 &pl_dims) {
    const glm::vec3 origo{-0.5f * pl_dims.x * pl_tan + -0.5f * pl_dims.y * pl_bitan};
    const glm::vec3 dir = pos - origo;
    return {glm::dot(dir, pl_tan) / pl_dims.x, glm::dot(dir, pl_bitan) / pl_dims.y};
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

/**
 * @brief Convert UVs into rectangular UVs
 * The mip maps are calculated in screen-space pixel coordinates, which is typically widescreen. So this function
 * counteracts the widescreen by transforming the uv-coordinates back to a rectangular format.
 *
 * TODO: Since the textures are only generated for the purpose of sampling them here, the textures could be
 * made quads in the mip map generation process instead, saving some calculations here.
 */
glm::vec2 rect_uvs(const glm::uvec2 &tex_dims, const glm::vec2 &uv) {
    const float x_min = 0.5f - static_cast<float>(tex_dims.y) / static_cast<float>(tex_dims.x + tex_dims.x);
    return {(1.f - x_min - x_min) * uv.x + x_min, uv.y};
}

// Bi-linear interpolation of pixel
glm::vec4 sample_tex(const glm::vec2 &uv, const glm::uvec2 tex_dims, const std::vector<glm::vec4> &tex_data) {
    const auto m_get_pixel = [tex_dims, &tex_data = std::as_const(tex_data)](const glm::uvec2 &coord) {
        return get_pixel(coord, tex_dims, tex_data);
    };

    const glm::vec2 pixel_coord = rect_uvs(tex_dims, uv) * glm::vec2{tex_dims + 1u};
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


/**
 * @brief The same as sample_tex, but only with height.
 * On a GPU most stuff is passed as a vec4 anyway, so interpolating vec4's and then swizzling the last component is
 * just as fast as interpolating singular floats. But on a CPU a singular component function could be optimized by the
 * compiler.
 */
float sample_height(const glm::vec2 &uv, const glm::uvec2 tex_dims, const std::vector<glm::vec4> &tex_data) {
    const auto m_get_pixel = [tex_dims, &tex_data = std::as_const(tex_data)](const glm::uvec2 &coord) {
        return get_pixel(coord, tex_dims, tex_data);
    };

    const glm::vec2 pixel_coord = rect_uvs(tex_dims, uv) * glm::vec2{tex_dims + 1u};
    const glm::vec2 f_pixel_coord = glm::fract(pixel_coord);
    const glm::uvec2 i_pixel_coord = glm::uvec2{pixel_coord - f_pixel_coord};

    auto aa = m_get_pixel(i_pixel_coord);
    auto ba = m_get_pixel(glm::uvec2{i_pixel_coord.x + 1u, i_pixel_coord.y});
    auto ab = m_get_pixel(glm::uvec2{i_pixel_coord.x, i_pixel_coord.y + 1u});
    auto bb = m_get_pixel(glm::uvec2{i_pixel_coord.x + 1u, i_pixel_coord.y + 1u});
    if (!aa || !ba || !ab || !bb)
        return 0.f;

    const auto s = std::lerp(
            std::lerp(aa->w, ba->w, f_pixel_coord.x),
            std::lerp(ab->w, bb->w, f_pixel_coord.x),
            f_pixel_coord.y);

    return std::isnan(s) ? 0.f : s;
}

/**
 * @brief Samples texture slices like a volume.
 * Using 2 slices, samples a density in a volume constructed from the slices, using the (un?)signed height differences
 * as the density. The xy-part of the sampling coordinates translates to the uv-coordinates in the plane, and the
 * z-coordinate translates to the difference between the 2 slices - the interpolation from the first layer to the
 * second layer.
 */
float sample_volume_tf(const glm::vec3 &coord, const TextureMipMaps &tex_mip_maps,
                       std::array<unsigned int, 2> levels, bool use_height_differences = false,
                       float mip_map_scale_mutliplier = 1.5f) {
    const auto &[dims0, data0] = tex_mip_maps.at(levels[0]);
    const auto &[dims1, data1] = tex_mip_maps.at(levels[1]);

    // If coordinates are outside of bounds, just return 0 as the density (air)
    // It is a logic error if we use the transfer function on invalid coords, so assert here:
    assert(!(glm::any(glm::lessThan(coord, glm::vec3{0.f})) || glm::any(glm::greaterThan(coord, glm::vec3{1.f}))));

    const auto t0 = sample_height(glm::vec2{coord}, dims0, data0) *
                    std::pow(mip_map_scale_mutliplier, static_cast<float>(levels[0]));
    const auto t1 = sample_height(glm::vec2{coord}, dims1, data1) *
                    std::pow(mip_map_scale_mutliplier, static_cast<float>(levels[1]));

    // (t1 - coord.z) - (t0 - coord.z) =
    return use_height_differences ? (t1 - t0) : std::lerp(t0, t1, coord.z);
}

// Sample a volume gradient using central differences
glm::dvec2 sample_volume_gradient(const glm::vec3 &coords, const TextureMipMaps &tex_mip_maps,
                                  std::array<unsigned int, 2> levels, float kernel_size = 0.001f,
                                  bool use_height_differences = false, float mip_map_scale_multiplier = 1.5f) {
    using namespace glm;
    const auto &tf = [&tex_mip_maps, levels, mip_map_scale_multiplier, use_height_differences](const vec3 &coord) {
        return sample_volume_tf(coord, tex_mip_maps, levels, use_height_differences, mip_map_scale_multiplier);
    };

    if (glm::any(glm::lessThan(coords, glm::vec3{0.f})) || glm::any(glm::greaterThan(coords, glm::vec3{1.f})))
        return {0.0, 0.0};

    /**
     * Volume gradient is central differences of transfer function in the X and Y directions, and an interpolated
     * constant upwards aligned vector in the Z-direction.
     */
    return {
            tf(vec3{std::min(coords.x + kernel_size, 1.f), coords.y, coords.z}) -
            tf(vec3{std::max(coords.x - kernel_size, 0.f), coords.y, coords.z}),
            tf(vec3{coords.x, std::min(coords.y + kernel_size, 1.f), coords.z}) -
            tf(vec3{coords.x, std::max(coords.y - kernel_size, 0.f), coords.z})
    };
}

glm::dvec2 surface_gradient(const glm::vec2 &uv, const glm::uvec2 &tex_dims, const std::vector<glm::vec4> &tex_data,
                            float kernel = 0.001f) {
    const auto tf = [&](const glm::vec2 &uv) { return sample_height(uv, tex_dims, tex_data); };
    // Note: Using the same kernel in x and y direction doesn't seem to change anything when using oblong textures
    return {
            tf({std::min(uv.x + kernel, 1.f), uv.y}) - tf({std::max(uv.x - kernel, 0.f), uv.y}),
            tf({uv.x, std::min(uv.y + kernel, 1.f)}) - tf({uv.x, std::max(uv.y - kernel, 0.f)})
    };
}

// Basically does the same on the CPU as calculateNormalFromHeightMap() from res/tiles/globals.glsl does on the GPU
glm::dvec3
surface_normal_from_gradient(const glm::vec2 &uv, const glm::uvec2 &tex_dims, const std::vector<glm::vec4> &tex_data) {
    using namespace glm;
    const auto g = surface_gradient(uv, tex_dims, tex_data) * 100.0; // Arbitrary scaling number for gradient
    return normalize(cross(normalize(dvec3{1.0, 0.0, g.x}), normalize(dvec3{0.0, 1.0, g.y})));
}

// Returns the soft "depth" into the surface. 0<= if outside surface and 1 if inside
float surface_depth(float height, float surface_softness = 0.f) {
    // Unsure about the coordinate system, so just setting max to 1 for now:
    constexpr float max_dist = -1.f;
    return surface_softness < 0.001f ? 1.f : std::min(height / (max_dist * surface_softness), 1.f);
//    return surface_softness < 0.001f ? 1.f : std::clamp(height / (max_dist * surface_softness), 0.f, 1.f);
}

glm::dvec3 soften_surface_normal(const glm::dvec3 &normal_force, float surface_depth, float min_force = 0.f) {
    const auto t = smoothstep(surface_depth);
    const float softness_interpolation = std::lerp(std::clamp(min_force, 0.f, 1.f), 1.f, t);

    return normal_force * static_cast<double>(softness_interpolation);
}

// Projects a point in world space onto the surface specified by a normal_force vector and
glm::dvec3 project_to_surface(const glm::dvec3 &world_pos, const glm::dvec3 &normal_force, float height) {
    /**
     * d = h * cos(alpha), cos(alpha) = dot({0, 0, h}, ||n||) / (|{0, 0, h}| * 1) = h * ||n||_z / h =>
     * d = h * ||n||_z
     * p' = p + ||n|| * d => p + ||n|| * h * ||n||_z
     */
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
    const auto [u_s, u_k] = std::make_pair(friction_scale, friction_scale * 0.5);
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

template<std::size_t I>
std::array<glm::dvec3, I> get_random_distributed_points_sphere(const glm::dvec3 &pos, double radius = 0.0001) {
    std::array<glm::dvec3, I> out;
    for (std::size_t i{0}; i < I; ++i)
        out.at(i) = pos + glm::ballRand(radius);
    return out;
}

using NormalLevelSampleResult = std::pair<float, std::optional<glm::dvec3>>;

/**
 * Samples the normal force, meaning the force pushing away from surface. In our case, this is the constraint force
 * of the haptic device.
 */
NormalLevelSampleResult
sample_normal_force(const glm::vec3 &relative_coords, const glm::uvec2 &tex_dims,
                    const std::vector<glm::vec4> &tex_data, float surface_height_multiplier = 1.f,
                    bool pre_interpolative = true) {
    const auto uv = glm::vec2{relative_coords};
    float height{0.f};
    glm::dvec3 normal{};
    if (pre_interpolative) {
        const auto h = sample_height(uv, tex_dims, tex_data);
        height = relative_coords.z - h * surface_height_multiplier;
        normal = surface_normal_from_gradient(uv, tex_dims, tex_data);
    } else {
        const auto value = sample_tex(uv, tex_dims, tex_data);
        height = relative_coords.z - value.w * surface_height_multiplier;
        normal = {value};
    }
    // If dist is positive, it means we're above the surface = no force applied
    if (0.f < height)
        return {height, std::nullopt};

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
    normal = norm < 0.0001 ? glm::dvec3{0., 0.0, 1.0} : normal * (1.0 / norm);

    return {height, glm::any(glm::isnan(normal)) ? std::nullopt : std::make_optional(normal)};
}

NormalLevelSampleResult sample_normal_level(const TextureMipMaps &tex_mip_maps,
                                            const SizedQueue<Physics::SimulationStepData, 2> &simulation_steps,
                                            glm::dvec3 coords, unsigned int level, float surface_height_multiplier,
                                            double surface_force, bool pre_interpolative = true) {
    const auto &[dims, data] = tex_mip_maps.at(level);

    const auto sample_res = sample_normal_force(coords, dims, data, surface_height_multiplier, pre_interpolative);
    const auto &[surface_height, opt_normal_force] = sample_res;

    // Early exit of there's no surface force (we're moving through air / empty space)
    if (!opt_normal_force)
        return sample_res;

    return {surface_height, {*opt_normal_force * surface_force}};
}

std::optional<Physics::NormalLevelResult>
sample_volume(double surface_force, float surface_softness, const TextureMipMaps &tex_mip_maps,
              const std::optional<float> &sphere_kernel_radius,
              const glm::vec3 &coords, unsigned int surface_volume_mip_map_counts, float t,
              bool use_height_differences = false, float mip_map_scale_multiplier = 1.5f,
              unsigned int min_mip_map = 0) {
    // Get upper and lower mip map levels:
    const auto enabled_mip_maps = generate_enabled_mip_maps(surface_volume_mip_map_counts, min_mip_map);
    const auto enabled_mip_maps_range_mult = static_cast<float>(surface_volume_mip_map_counts - 1);

    // t(z) = [0, 1], z = [-0.25, 0.25]
    const auto upper_j = static_cast<std::size_t>(std::ceil(t * enabled_mip_maps_range_mult));
    const auto lower_j = static_cast<std::size_t>(std::floor(t * enabled_mip_maps_range_mult));
    auto upper_level = enabled_mip_maps.at(upper_j);
    auto lower_level = enabled_mip_maps.at(lower_j);

    const auto f_f = t * enabled_mip_maps_range_mult - static_cast<float>(lower_j);

    glm::dvec2 gradient = sample_volume_gradient(glm::vec3{coords.x, coords.y, f_f}, tex_mip_maps,
                                      std::to_array({lower_level, upper_level}),
                                      sphere_kernel_radius ? *sphere_kernel_radius : 0.001f, use_height_differences,
                                      mip_map_scale_multiplier);
    // Multiply 2D gradient with an arbitrary scaling number
    gradient *= surface_force * -100.0;

    const double depth = 1.0 - t;
    // ease out function based on http://gizma.com/easing/
    // -x * (x-2) = 1 - (x-1)^2 => 1 + (x-1)^3
    glm::dvec3 force{gradient, surface_force * (1.0 + std::pow(depth - 1.0, 3.0))};

    constexpr double EPSILON = 0.0001;
    const double g_len = glm::length(force);
    if (g_len <= EPSILON)
        return std::nullopt;

    // Clamp down (for safety)
    if (surface_force < g_len)
        force *= surface_force / g_len;

    const float h = t - 1.f;
    return {{.normal = force, .soft_normal = soften_surface_normal(force, surface_depth(h,
                                                                                        surface_softness)), .height = h}};
}

bool
outside_surface(const glm::dvec3 &pos, const glm::dvec3 &pl_norm, const glm::dvec3 &pl_pos, float surface_softness) {
    constexpr float OUTSIDE_SURFACE_DEPTH_THRESHOLD = 0.1f;
    const auto h = surface_depth(point_to_plane(pos, pl_norm, pl_pos), surface_softness);
    return h < OUTSIDE_SURFACE_DEPTH_THRESHOLD;
}

namespace molumes {
    glm::dvec3 Physics::simulate_and_sample_force(double surface_force, float surface_softness,
                                                  float surface_height_multiplier, unsigned int mip_map_level,
                                                  const TextureMipMaps &tex_mip_maps, glm::dvec3 pos,
                                                  std::optional<float> friction_scale,
                                                  std::optional<float> gravity_factor,
                                                  std::optional<unsigned int> surface_volume_mip_map_counts,
                                                  std::optional<float> sphere_kernel_radius,
                                                  bool volume_use_height_differences,
                                                  float mip_map_scale_multiplier, bool pre_interpolative_normal,
                                                  bool intersection_constraint) {
        PROFILE("Physics - Sample force");

        /**
         * Currently just hardcoding a xy-plane lying in origo, and then rotating that plane to be a xz-plane
         * (it was easier to work with the xy plane while doing the physics calculations)
         */
        const static glm::dmat4 pl_r_mat{{1.0, 0.0, 0.0,  0.0},
                                         {0.0, 0.0, -1.0, 0.0},
                                         {0.0, 1.0, 0.0,  0.0},
                                         {0.0, 0.0, 0.0,  1.0}};
        const static glm::dmat4 pl_ri_mat{glm::inverse(pl_r_mat)};
        pos = glm::dvec3{pl_ri_mat * glm::dvec4{pos, 1.0}};
        /**
         * Note: It seems matrix multiplication is very expensive (about 1/4 increased time), so a big performance
         * boost would be to not multiply with rotation matrices before and after, but instead just to do the whole
         * force calculation in the correct space.
         *
         * Without matrix mult, avg: 11754ns
         * With matrix mult, avg: 14777ns
         */

        auto &current_simulation_step = create_simulation_record(pos);
        const static glm::mat4 pl_mat{1.f};

        glm::dvec3 sum_forces{0.0};

        // Optional gravity force (always constant, does not care whether inside bounds or not)
        if (gravity_factor)
            sum_forces.z -= *gravity_factor;

        const auto coords = opt_relative_pos_coords(glm::vec3{pos}, pl_mat, glm::vec2{2.f});
        if (!coords) {
            // Apply a constant up-facing surface force outside of bounds (simulating an infinite plane)
            sum_forces += soften_surface_normal(glm::dvec3{0.0, 0.0, surface_force}, surface_depth(
                    point_to_plane(pos, glm::vec3{pl_mat[2]}, glm::vec3{pl_mat[3]}), surface_softness));
            return {pl_r_mat * glm::dvec4{sum_forces, 0.0}};
        }

        // Shouldn't be possible for coords to be negative
        assert(!glm::any(glm::lessThan(glm::vec2{*coords}, glm::vec2{0.f})) ||
               !glm::any(glm::greaterThan(glm::vec2{*coords}, glm::vec2{1.f})));

        // Possible TODO: Monte carlo sampling somewhere around here (before t)

        // Surface volume (multiple surfaces)
        std::optional<NormalLevelResult> sample_level_results;
        // Monte-carlo sampling:
        if (sphere_kernel_radius) {
            static constexpr std::size_t SAMPLE_SIZE = 8;
            const auto points = get_random_distributed_points_sphere<SAMPLE_SIZE>(*coords, *sphere_kernel_radius);
            double count{0};
            sample_level_results = std::make_optional(NormalLevelResult{});
            for (auto i{0u}; i < SAMPLE_SIZE; ++i) {
                const auto res = sample_normal(points.at(i), surface_force, surface_softness, surface_height_multiplier,
                                               mip_map_level, tex_mip_maps, pos, surface_volume_mip_map_counts,
                                               sphere_kernel_radius, volume_use_height_differences,
                                               mip_map_scale_multiplier, pre_interpolative_normal,
                                               intersection_constraint);
                if (res) {
                    *sample_level_results += *res;
                    count += 1.0;
                }
            }

            sample_level_results = 0.0 < count ?
                                   std::make_optional(NormalLevelResult{
                                           .normal = sample_level_results->normal / count,
                                           .soft_normal = sample_level_results->soft_normal / count,
                                           .height = sample_level_results->height / static_cast<float>(count)})
                                               : std::nullopt;
        } else
            sample_level_results = sample_normal(*coords, surface_force, surface_softness, surface_height_multiplier,
                                                 mip_map_level, tex_mip_maps, pos, surface_volume_mip_map_counts,
                                                 sphere_kernel_radius, volume_use_height_differences,
                                                 mip_map_scale_multiplier, pre_interpolative_normal,
                                                 intersection_constraint);

        // We're above the (all) surface(s). In the air, return early.
        if (!sample_level_results)
            return {pl_r_mat * glm::dvec4{sum_forces, 0.0}};

        auto [normal_force, soft_normal_force, surface_height] = *sample_level_results;
        current_simulation_step.normal_force = normal_force;
        current_simulation_step.surface_height = surface_height;

        // Check if still inside surface
        if (intersection_constraint)
            update_itnersecting(surface_softness, pos, current_simulation_step, normal_force, surface_height);

        sum_forces += soft_normal_force;

        if (friction_scale) {
            const auto surface_pos = project_to_surface(pos, normal_force, surface_height);
            const auto friction = calc_uniform_friction(m_simulation_steps, surface_pos,
                                                        static_cast<double>(*friction_scale),
                                                        soft_normal_force);
            sum_forces += friction;
        }

        return {pl_r_mat * glm::dvec4{sum_forces, 0.0}};
    }

    std::optional<Physics::NormalLevelResult>
    Physics::sample_normal(const glm::vec3 &coords, double surface_force, float surface_softness,
                           float surface_height_multiplier, unsigned int mip_map_level,
                           const TextureMipMaps &tex_mip_maps, const glm::dvec3 &pos,
                           const std::optional<unsigned int> &surface_volume_mip_map_counts,
                           const std::optional<float> &sphere_kernel_radius,
                           bool volume_use_height_differences, float mip_map_scale_multiplier,
                           bool pre_interpolative_normal, bool intersection_constraint) {
        constexpr float VOLUME_MAX_FORCE = 0.5f;

        const bool volume_enabled = surface_volume_mip_map_counts.has_value();
        if (volume_enabled) {
            float t{coords.z * 2.f + 0.5f}; // z = [-0.25, 0.25]

            // If above volume, we're in air. Return early
            if (1.f < t)
                return {};

            // If inside volume:
            if (0.f < t) {
                // Check if when we sample the actual height we're still inside the volume
                const auto &[floor_dims, floor_data] = tex_mip_maps.at(mip_map_level);
                const auto floor_height =
                        sample_height({coords}, floor_dims, floor_data) * surface_height_multiplier -
                        0.25f;
                const float t_h = coords.z / (0.25f - floor_height) - floor_height / (0.25f - floor_height);

                // If the actual height says we're still inside the volume:
                if (0.f < t_h)
                    return sample_volume(surface_force * VOLUME_MAX_FORCE, surface_softness, tex_mip_maps,
                                         sphere_kernel_radius, coords, *surface_volume_mip_map_counts, t_h,
                                         volume_use_height_differences, mip_map_scale_multiplier, mip_map_level);
            }
        }

        // Singular surface:
        const glm::dvec3 c{coords.x, coords.y, coords.z + (volume_enabled ? 0.25f : 0.f)};
        const auto [h, opt_norm] = sample_normal_level(tex_mip_maps, m_simulation_steps, c, mip_map_level,
                                                       surface_height_multiplier, surface_force,
                                                       pre_interpolative_normal);

        const auto &last_step = m_simulation_steps.get_from_back<1>();
        const auto min_soft_force = volume_enabled ? VOLUME_MAX_FORCE : 0.f;

        // If we passed through the surface last frame, use last normal
        if (intersection_constraint && last_step.intersection_plane) {
            const auto plane_depth = surface_depth(
                    point_to_plane(pos, last_step.intersection_plane->normal, last_step.intersection_plane->pos),
                    surface_softness);
            return {{last_step.normal_force, soften_surface_normal(last_step.normal_force, plane_depth, min_soft_force),
                     h}};
        } else {
            return optional_chain(opt_norm, [=, h = h](const auto &n) -> std::optional<NormalLevelResult> {
                return {{n, soften_surface_normal(n, surface_depth(h, surface_softness), min_soft_force), h}};
            });
        }
    }

    void Physics::update_itnersecting(float surface_softness, const glm::dvec3 &pos,
                                      Physics::SimulationStepData &current_simulation_step,
                                      const glm::dvec3 &normal_force, float surface_height) {
        const auto last_simulation_step = m_simulation_steps.get_from_back<1>();
        constexpr float INSIDE_SURFACE_DEPTH_THRESHOLD = 0.9f;
        const auto depth = surface_depth(surface_height, surface_softness);
        const auto was_inside = last_simulation_step.intersection_plane.has_value();
        if (was_inside && !outside_surface(pos, last_simulation_step.intersection_plane->normal,
                                           last_simulation_step.intersection_plane->pos, surface_softness)) {
            current_simulation_step.intersection_plane = last_simulation_step.intersection_plane;
        } else if (!was_inside && INSIDE_SURFACE_DEPTH_THRESHOLD < depth) {
            const auto n = glm::normalize(normal_force);
//            current_simulation_step.intersection_plane = {{.normal = n, .pos = n * static_cast<double>(1.f - depth) + pos}};
            /* Projecting in the direction of the normal runs the risk of placing a plane lower than the surface point,
             * as the normal direction is an estimation of the surface and not completely accurate. Instead, just use
             * position + the surface height as the position of the plane.
             */
            current_simulation_step.intersection_plane = {
                    {.normal = n, .pos = pos + glm::dvec3{0.0, 0.0, -surface_height}}
            };
        }
    }

    std::vector<unsigned int> generate_enabled_mip_maps(unsigned int enabled_count, unsigned int min_mip_map) {
        std::vector<unsigned int> out;
        out.reserve(enabled_count);
        for (unsigned int i{0}; i < enabled_count; ++i)
            out.push_back(static_cast<unsigned int>(std::round(
                    static_cast<float>((HapticMipMapLevels - 1 - min_mip_map) * i) /
                    static_cast<float>(enabled_count - 1) + 0.01f
            )) + min_mip_map);
        return out;
    }

}