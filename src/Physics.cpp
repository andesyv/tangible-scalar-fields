#include "Physics.h"

#include <numbers>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


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

// Uses the quaternion between two vectors to spherically interpolate between them. (quite more expensive than lerp)
template<typename T>
glm::vec<3, T, glm::defaultp>
slerp(const glm::vec<3, T, glm::defaultp> &a, const glm::vec<3, T, glm::defaultp> &b, T t) {
    const auto q = glm::angleAxis(t * std::acos(glm::dot(a, b)), glm::normalize(glm::cross(a, b)));
    return q * a;
}

glm::vec4 scaling_mix(const glm::vec4 &a, const glm::vec4 &b, float t) {
    const auto diff = b.w - a.w;
    if (0.001f < std::abs(diff))
        return glm::vec4{
                slerp(glm::normalize(glm::vec3{a.x, a.y, a.z / diff}), glm::normalize(glm::vec3{b.x, b.y, b.z / diff}),
                      t), // Maybe unnecessary to normalize here
                std::lerp(a.w, b.w, t)
        };
    else
        return glm::vec4{
                slerp(glm::vec3{a}, glm::vec3{b}, t), // Maybe unnecessary to normalize here
                std::lerp(a.w, b.w, t)
        };
};

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

auto estimate_user_force(std::array<molumes::SimulationStepData, 2> last_steps, const glm::dvec3 &normal_force,
                         double friction_scale, const glm::dvec3 &up = glm::dvec3{0.0, 1.0, 0.0}) {
    /** If the end-effector is stationary situated on the surface, the applied force can be found from the normal force
     * pushing away from the surface:
     * F_n = F_u * cos(angle) => F_u = F_n / cos(angle) => F_u = F_n / dot(|n|, |up|)
     *
     * F = F_u + F_n + F_fr = a * m => F_u = a * m - F_n - F_fr = a * m - F_n - F_n * u
     * F_u = a * m - F_n (1 + u)
     */

    const auto n_len = glm::length(normal_force);
    const auto n = normal_force * (1.0 / n_len);
    return n_len / glm::dot(n, up);
}

/**
 * Samples the normal force, meaning the force pushing away from surface. In our case, this is the constraint force
 * of the haptic device.
 * @return A dvec3 containing normal force, or std::nullopt if there's no force
 */
std::optional<glm::dvec3> sample_normal_force(const glm::vec3 &relative_coords, const glm::uvec2 &tex_dims,
                                              const std::vector<glm::vec4> &tex_data,
                                              glm::uint mip_map_level = 0, float surface_softness = 0.f,
                                              float surface_height = 1.f) {
    const auto value = sample_tex(glm::vec2{relative_coords}, tex_dims, tex_data);
    float height = relative_coords.z - value.w * surface_height;
    // If dist is positive, it means we're above the surface = no force applied
    if (0.f < height)
        return std::nullopt;

    // Unsure about the coordinate system, so just setting max to 1 for now:
    static constexpr float max_dist = 1.f;
    const float softness_interpolation =
            surface_softness < 0.001f ? 1.f : smoothstep(0.f, 1.f,
                                                         std::min(height / (-max_dist * surface_softness), 1.f));

    // Surface normal
    // At this point the normal has been scaled with the kde value of the surface
    glm::vec3 normal{value};

    /**
     * Modulate surface normal with surface scale:
     * Surface normal is uniformly scaled in the plane axis, so the scaling acts as a regular scaling model matrix.
     * But since these are normal vectors along the surface, they should be treated the same way as a
     * "computer graphics normal vector", meaning they should be multiplied with the "normal matrix", the transpose
     * inverse of the model matrix:
     * N = (M^-1)^T => (({[1, 0, 0], [0, 1, 0], [0, 0, S]})^-1)^T
     *  => [x, y, z] -> [x, y, z / S]
     */
    normal.z /= surface_height;

    glm::dvec3 d_normal{normal};

    // Normalize:
    const auto norm = glm::length(d_normal);
    d_normal = norm < 0.0001 ? glm::dvec3{0., 0.f, 1.0} : d_normal * (1.0 / norm);

    if (glm::any(glm::isnan(d_normal)))
        return std::nullopt;


    // Find distance to surface (not the same as height, as that is projected down):
//    const auto dist = -height * normal.z; // dist = dot(vec3{0., 0., 1.}, normal) * -height;

    return std::make_optional(glm::dvec3{normal * softness_interpolation});
}

//auto sphere_sample_surface_force(float sphere_radius, const glm::vec3 &pos,
//                                 const glm::uvec2 &tex_dims, const std::vector<glm::vec4> &tex_data,
//                                 glm::uint mip_map_level = 0, float surface_softness = 0.f) {
//    constexpr float angle_piece = std::numbers::pi_v<float> / 8.f; // How is this not a thing before C++20 ???
//
//    const glm::mat4 pl_mat{1.0}; // Currently just hardcoding a xy-plane lying in origo
//    const auto coords = relative_pos_coords(pos - glm::vec3{0.f, 0.f, sphere_radius}, pl_mat, glm::vec2{2.f});
//
//    glm::vec3 sum = sample_surface_force(coords, tex_dims, tex_data, mip_map_level,
//                                         surface_softness);
//
//    for (int i = 0; i < 8; ++i) {
//        const auto angle = angle_piece * static_cast<float>(i);
//        const auto displacement = glm::vec3{std::sin(angle), std::cos(angle), 0.f} * sphere_radius;
//        const auto coord = opt_relative_pos_coords(pos + displacement, pl_mat, glm::vec2{2.f});
//        if (coord)
//            sum += sample_surface_force(*coord, tex_dims, tex_data, mip_map_level, surface_softness);
//    }
//    return sum * (1.f / 9.f);
//}

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

/** General plan:
 * 1. Estimate user velocity (dhdGetLinearVelocity() )
 * 2. Estimate user force applied to haptic device (using estimated velocity): F_u
 * 2.2 Optionally apply gravity force: F_g
 * (haptic device applies force to make the haptic device stationary, meaning we can add artificial gravity if we'd like)
 * 3. Find surface normal: n
 * 4. Calculate normal (constraint) force. F_n = dot(F_u, n)
 * 5. Calculate friction force:
 * F_f = F_f_dir * F_f_len = (-||F_u + F_n||) * (|F_n| * (if |v| < threshold then u_s else u_k)), where u_k < u_s
 * 6. Apply sum of forces: F = F_n + F_f + F_g
 */
glm::dvec3 molumes::sample_force(float max_force, float surface_softness, float friction_scale, float surface_height,
                                 FrictionMode friction_mode, unsigned int mip_map_level,
                                 const TextureMipMaps &tex_mip_maps,
                                 const glm::dvec3 &pos, std::array<SimulationStepData, 2> last_steps,
                                 const glm::dmat3 &haptic_mat) {
    const glm::mat4 pl_mat{1.0}; // Currently just hardcoding a xy-plane lying in origo

    const auto coords = opt_relative_pos_coords(glm::vec3{pos}, pl_mat, glm::vec2{2.f});
    // Can't calculate a force outside the bounds of the plane, so return no force
    if (!coords)
        return glm::vec3{0.f};

    // Shouldn't be possible for coords to be negative
    assert(!glm::any(glm::lessThan(glm::vec2{*coords}, glm::vec2{0.f})));

    const auto&[dims, data] = tex_mip_maps.at(mip_map_level);

    glm::dvec3 sum_forces{0.0};

    // 3-4. Calculate normal (constraint) force
    const auto normal_force = sample_normal_force(*coords, dims, data, mip_map_level, surface_softness,
                                                  surface_height);

    // Early exit of there's no surface force (we're moving through air / empty space)
    if (!normal_force)
        return glm::dvec3{0.0};

    sum_forces += *normal_force;

    // 1-2. Estimate user force
    const auto user_force_len = estimate_user_force(last_steps, *normal_force, friction_scale);

    // Note: Currently only uniform friction is implemented
    if (friction_mode == FrictionMode::Uniform) {
        const auto v_len = glm::length(last_steps.back().velocity);
        // Temporarily just hard coding the kinetic friction scale. It is required that u_k < u_s, but they can both be
        // arbitrary set to anything that fulfills this requirement.
        const auto[u_s, u_k] = std::make_pair(static_cast<double>(friction_scale),
                                              static_cast<double>(friction_scale) * 0.5);
        const auto friction_force = calc_friction(v_len < 0.001 ? u_s : u_k, user_force, *normal_force);
        if (friction_force)
            sum_forces += *friction_force;
    }

    // Optional gravity force:
    // sum_forces += glm::dvec3{0.0, 0.0, -9.81};

    /**
     * Clamp force to max_force:
     * Note: The force scaling should (in the future) come from the user_force via a user_mass scaling parameter, so it
     * should only be necessary to clamp it to the physical limit (9 Newton) of the device here.
     */
    const auto f_len = glm::length(sum_forces);
    return max_force < f_len ? (sum_forces * (max_force / f_len)) : sum_forces;
}