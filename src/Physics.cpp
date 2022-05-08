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

template<typename T, typename R>
std::optional<R> optional_chain(const std::optional<T> &opt) {
    return opt ? std::make_optional<R>(*opt) : std::nullopt;
}

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

auto pos_uvs_in_plane(const glm::vec3 &pos, const glm::mat4 &pl_mat, const glm::vec2 &pl_dims) {
    return pos_uvs_in_plane(pos, glm::vec3{pl_mat[0]}, glm::vec3{pl_mat[1]}, pl_dims);
}

// returns uv as xy and signed distance as z for a position given a orientation matrix for a plane and it's dimensions
glm::vec3 relative_pos_coords(const glm::vec3 &pos, const glm::mat4 &pl_mat, const glm::vec2 &pl_dims) {
    return {
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
    const auto &tf = [&tex_mip_maps, levels, mip_map_scale_multiplier](const vec3 &coord) {
        return sample_volume_tf(coord, tex_mip_maps, levels, mip_map_scale_multiplier);
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
//    return glm::cross(glm::dvec3{1.0, 0.0, samples[0] && samples[1] ? *samples[0] - *samples[1] : 0.0}, glm::dvec3{0.0, 1.0, samples[2] && samples[3] ? *samples[2] - *samples[3] : 0.0});
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

auto get_relative_height(const glm::vec3 &relative_coords, const glm::uvec2 &tex_dims,
                         const std::vector<glm::vec4> &tex_data, float surface_height_multiplier = 1.f) {
    const auto value = sample_height(glm::vec2{relative_coords}, tex_dims, tex_data);
    return relative_coords.z - value * surface_height_multiplier;
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

    const auto &[dims, data] = tex_mip_maps.at(level);
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

template<std::size_t I>
std::array<glm::dvec3, I> get_random_distributed_points_sphere(const glm::dvec3 &pos, double radius = 0.0001) {
    std::array<glm::dvec3, I> out;
    for (std::size_t i{0}; i < I; ++i)
        out.at(i) = pos + glm::ballRand(radius);
    return out;
}

template<std::size_t I, typename F, typename ... Args>
auto monte_carlo_sample_arr(const glm::dvec3 &pos, double radius, F &&func, Args &&... args) {
    using R = decltype(func(pos, args...));
    std::array<R, I> out;
    const auto points = get_random_distributed_points_sphere<I>(pos, radius);
    for (auto i{0u}; i < I; ++i)
        out.at(i) = func(points.at(i), std::forward<Args>(args)...);
    return out;
}

// Invokes a function will the call syntax F(pos, args), I times and returns the average result
template<std::size_t I, typename F, typename ... Args>
auto monte_carlo_sample(const glm::dvec3 &pos, double radius, F &&func, Args &&... args) {
    using R = decltype(func(pos, args...));
    const auto samples = monte_carlo_sample_arr<I>(pos, radius, std::forward<F>(func), std::forward<Args>(args)...);
    return std::accumulate(samples.begin(), samples.end(), R{}) * (1.0 / static_cast<double>(I));
}

using NormalLevelSampleResult = std::pair<float, std::optional<glm::dvec3>>;

/**
 * Samples the normal force, meaning the force pushing away from surface. In our case, this is the constraint force
 * of the haptic device.
 * @return A dvec3 containing normal force, or std::nullopt if there's no force
 */
NormalLevelSampleResult
sample_normal_force(const glm::vec3 &relative_coords, const glm::uvec2 &tex_dims,
                    const std::vector<glm::vec4> &tex_data, glm::uint mip_map_level = 0,
                    float surface_height_multiplier = 1.f, bool normal_offset = false, bool pre_interpolative = true) {
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
//    if (0.f < height)
//        return {height, std::nullopt};

//    if (normal_offset) {
//        /**
//         * Sample the normal at an offset from the height, using the sampled normal as an offset basis. The idea is that
//         * the offset surface point is (hopefully) closer to the projected point.
//         */
//        const auto offset = 2.f * glm::vec2{value} / glm::vec2{tex_dims};
//        const auto offset_value = sample_tex(glm::vec2{relative_coords} - offset, tex_dims, tex_data);
//        normal = {offset_value};
//    }

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

struct NormalLevelResult {
    glm::dvec3 normal, soft_normal;
    float height;
};

NormalLevelSampleResult
sample_normal_level(const TextureMipMaps &tex_mip_maps,
                    const SizedQueue<Physics::SimulationStepData, 2> &simulation_steps,
                    glm::dvec3 coords, unsigned int level, float surface_height_multiplier, bool normal_offset,
                    double surface_force, float surface_softness, bool pre_interpolative = true) {
    const auto &[dims, data] = tex_mip_maps.at(level);

//    // Find a better estimation to closest point:
//    coords = find_closest_point_in_sphere(coords, 0.00001, tex_mip_maps, level,
//                                          surface_height_multiplier);


    // 3-4. Calculate normal (constraint) force
    NormalLevelSampleResult sample_res{};
    auto &[surface_height, opt_normal_force] = sample_res;
//    if (monte_carlo_sampling) {
//        /// A bit convoluted with optional sums and stuff, but should work
//        const auto samples = monte_carlo_sample_arr<16>(coords, 0.0001, sample_normal_force, dims, data, level,
//                                                        surface_height_multiplier, normal_offset);
//        auto count = 0u;
//        for (const auto &[h, opt_n]: samples) {
//            if (opt_n) {
//                sample_res = {0u < count ? surface_height + h : h, {0u < count ? *opt_normal_force + *opt_n : *opt_n}};
//                ++count;
//            }
//        }
//        const auto c = static_cast<float>(count);
//        if (0u < count) {
//            surface_height /= c;
//            *opt_normal_force *= (1.0 / c);
//        }
//    } else

    sample_res = sample_normal_force(coords, dims, data, level, surface_height_multiplier, normal_offset,
                                     pre_interpolative);

    // Early exit of there's no surface force (we're moving through air / empty space)
    if (!opt_normal_force)
        return sample_res;

    // Scale normal force with normal_force:
    auto normal_force = *opt_normal_force * surface_force;

//    // Adjust normal by using the half of the last normal_vector (smoothes out big changes in normal, like a moving over a bump)
//    auto half_normal = simulation_steps.get_from_back<1>().normal_force + normal_force;
//    const auto half_normal_len = glm::length(half_normal);
//    if (0.001 < half_normal_len) {
//        half_normal *= surface_force / half_normal_len;
//        normal_force = half_normal;
//    }

    return {surface_height, {normal_force}};
}

std::optional<NormalLevelResult>
sample_volume(double surface_force, float surface_softness, const TextureMipMaps &tex_mip_maps,
              const std::optional<float> &sphere_kernel_radius, bool monte_carlo_sampling,
              const glm::vec3 &coords, unsigned int surface_volume_mip_map_counts, float t,
              bool use_height_differences = false, float mip_map_scale_multiplier = 1.5f) {
    // Get upper and lower mip map levels:
    const auto enabled_mip_maps = generate_enabled_mip_maps(surface_volume_mip_map_counts);
    const auto enabled_mip_maps_range_mult = static_cast<float>(surface_volume_mip_map_counts - 1);

    // t(z) = [0, 1], z = [-0.25, 0.25]
    const auto upper_j = static_cast<std::size_t>(std::ceil(t * enabled_mip_maps_range_mult));
    const auto lower_j = static_cast<std::size_t>(std::floor(t * enabled_mip_maps_range_mult));
    auto upper_level = static_cast<unsigned int>(enabled_mip_maps.at(upper_j));
    auto lower_level = static_cast<unsigned int>(enabled_mip_maps.at(lower_j));

    const auto f_f = t * enabled_mip_maps_range_mult - static_cast<float>(lower_j);

    glm::dvec2 g_2d{};
    /// Monte carlo sampling should happen at the level above
    /// (monte-carlo sampling should be able to happen between different mip map levels)
//    if (monte_carlo_sampling) {
//        g_2d = monte_carlo_sample<8>(glm::vec3{coords.x, coords.y, f_f}, 0.0001,
//                                     sample_volume_gradient,
//                                     tex_mip_maps, std::to_array({lower_level, upper_level}),
//                                     sphere_kernel_radius ? *sphere_kernel_radius : 0.001f, use_height_differences);
//    } else {
//    }
    g_2d = sample_volume_gradient(glm::vec3{coords.x, coords.y, f_f}, tex_mip_maps,
                                  std::to_array({lower_level, upper_level}),
                                  sphere_kernel_radius ? *sphere_kernel_radius : 0.001f, use_height_differences,
                                  mip_map_scale_multiplier);
    g_2d *= surface_force * -100.0;

    const double depth = 1.0 - t;
    // -x * (x-2) = 1 - (x-1)^2 = ease out function: http://gizma.com/easing/
    glm::dvec3 gradient{g_2d, surface_force * (1.f + std::pow(depth - 1.f, 3.f))};

    constexpr double EPSILON = 0.0001;
    const double g_len = glm::length(gradient);
    if (g_len <= EPSILON)
        return std::nullopt;

    // Clamp down (for safety)
    if (surface_force < g_len)
        gradient *= surface_force / g_len;

    const float h = t - 1.f;
    return {{.normal = gradient, .soft_normal = soften_surface_normal(gradient, surface_depth(h,
                                                                                              surface_softness)), .height = h}};
}

bool outside_surface(const glm::dvec3& pos, const glm::dvec3& pl_norm, const glm::dvec3& pl_pos, float surface_softness) {
    constexpr float OUTSIDE_SURFACE_DEPTH_THRESHOLD = 0.1f;
    const auto h = surface_depth(point_to_plane(pos, pl_norm, pl_pos), surface_softness);
    return h < OUTSIDE_SURFACE_DEPTH_THRESHOLD;
}

namespace molumes {
    glm::dvec3 Physics::simulate_and_sample_force(double surface_force, float surface_softness, float friction_scale,
                                                  float surface_height_multiplier, FrictionMode friction_mode,
                                                  unsigned int mip_map_level, const TextureMipMaps &tex_mip_maps,
                                                  glm::dvec3 pos, bool normal_offset,
                                                  std::optional<float> gravity_factor,
                                                  std::optional<unsigned int> surface_volume_mip_map_counts,
                                                  std::optional<float> sphere_kernel_radius,
                                                  bool linear_volume_surface_force, bool monte_carlo_sampling,
                                                  double volume_z_multiplier, bool volume_use_height_differences,
                                                  float mip_map_scale_multiplier, bool pre_interpolative_normal) {
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

        std::optional<NormalLevelResult> sample_level_results;
        constexpr float VOLUME_MAX_FORCE = 0.5f;
        // Possible TODO: Monte carlo sampling somewhere around here (before t)

        // Surface volume (multiple surfaces)
        if (surface_volume_mip_map_counts) {
            float t{coords->z * 2.f + 0.5f}, t_h{t}; // z = [-0.25, 0.25]
            bool above_volume = 1.f < t;
            bool inside_volume = !above_volume && 0.f < t;

            if (inside_volume) {
                const auto &[floor_dims, floor_data] = tex_mip_maps.at(mip_map_level);
                const auto floor_height =
                        sample_height({coords->x, coords->y}, floor_dims, floor_data) * surface_height_multiplier -
                        0.25f;
                t_h = coords->z / (0.25f - floor_height) - floor_height / (0.25f - floor_height);
                inside_volume = 0.f < t_h;
            }

            if (inside_volume) {
                sample_level_results = sample_volume(surface_force * VOLUME_MAX_FORCE, surface_softness,
                                                     tex_mip_maps, sphere_kernel_radius,
                                                     monte_carlo_sampling, *coords, *surface_volume_mip_map_counts, t_h,
                                                     volume_use_height_differences, mip_map_scale_multiplier);
            } else if (!above_volume) {
                const glm::dvec3 c{coords->x, coords->y, coords->z + 0.25f};
                const auto [h, opt_norm] = sample_normal_level(tex_mip_maps, m_simulation_steps, c,
                                                               mip_map_level, surface_height_multiplier,
                                                               normal_offset, surface_force, surface_softness);
                sample_level_results = optional_chain(opt_norm,
                                                      [=, h = h](const auto &n) -> std::optional<NormalLevelResult> {
                                                          return {{n, soften_surface_normal(n, surface_depth(h,
                                                                                                             surface_softness),
                                                                                            VOLUME_MAX_FORCE), h}};
                                                      });
            }

            // Singular surface:
        } else {
            const auto [h, opt_norm] = sample_normal_level(tex_mip_maps, m_simulation_steps, *coords,
                                                           mip_map_level, surface_height_multiplier,
                                                           normal_offset, surface_force, surface_softness,
                                                           pre_interpolative_normal);
            sample_level_results = optional_chain(opt_norm, [=, h = h](const auto &n) -> std::optional<NormalLevelResult> {
                const auto &last_step = m_simulation_steps.get_from_back<1>();
//                const auto last_depth = surface_depth(last_step.surface_height, surface_softness);
                const auto current_depth = surface_depth(h, surface_softness);
                // If we passed through the surface last frame, use last normal
                if (last_step.intersection_plane) {
                    const auto plane_depth = surface_depth(point_to_plane(pos, last_step.intersection_plane->normal, last_step.intersection_plane->pos), surface_softness);
                  return {{last_step.normal_force, soften_surface_normal(last_step.normal_force, plane_depth), h}};
                } else
                  return {{n, soften_surface_normal(n, current_depth), h}};
            });
        }

        // We're above the (all) surface(s). In the air, return early.
        if (!sample_level_results)
            return {pl_r_mat * glm::dvec4{sum_forces, 0.0}};

        auto [normal_force, soft_normal_force, surface_height] = *sample_level_results;
        current_simulation_step.normal_force = normal_force;
        current_simulation_step.surface_height = surface_height;

        // Check if still inside surface
        const auto last_simulation_step = m_simulation_steps.get_from_back<1>();
        constexpr float INSIDE_SURFACE_DEPTH_THRESHOLD = 0.9f;
        const auto depth = surface_depth(surface_height, surface_softness);
        const auto was_inside = last_simulation_step.intersection_plane.has_value();
        if (was_inside && !outside_surface(pos, last_simulation_step.intersection_plane->normal, last_simulation_step.intersection_plane->pos, surface_softness)) {
            current_simulation_step.intersection_plane = last_simulation_step.intersection_plane;
        } else if (!was_inside && INSIDE_SURFACE_DEPTH_THRESHOLD < depth) {
            const auto n = glm::normalize(normal_force);
//            current_simulation_step.intersection_plane = {{.normal = n, .pos = n * static_cast<double>(1.f - depth) + pos}};
            /* Projecting in the direction of the normal runs the risk of placing a plane lower than the surface point,
             * as the normal direction is an estimation of the surface and not completely accurate. Instead, just use
             * position + the surface height as the position of the plane.
             */
            current_simulation_step.intersection_plane = {{.normal = n, .pos = pos + glm::dvec3{0.0, 0.0, -surface_height}}};
        }

        sum_forces += soft_normal_force;

        // Note: Currently only uniform friction is implemented
        if (friction_mode == FrictionMode::Uniform) {
            const auto surface_pos = project_to_surface(pos, normal_force, surface_height);
            const auto friction = calc_uniform_friction(m_simulation_steps, surface_pos,
                                                        static_cast<double>(friction_scale),
                                                        soft_normal_force);
            sum_forces += friction;
        }

        return {pl_r_mat * glm::dvec4{sum_forces, 0.0}};
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