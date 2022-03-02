#ifndef MOLUMES_PHYSICS_H
#define MOLUMES_PHYSICS_H

#include "Constants.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat3x3.hpp>

#include <array>
#include <vector>
#include <chrono>

namespace molumes {
    // Data structure to record previous simulation data
    struct SimulationStepData {
        std::chrono::high_resolution_clock::duration::rep delta_us; // time since last recording in microseconds
        glm::dvec3 pos; // Current position
        glm::dvec3 velocity; // Delta velocity from last frame (to current pos)
        glm::dvec3 acceleration;
        glm::dvec3 normal_force;
    };

    enum FrictionMode {
        Disabled,
        Uniform
        // Directional
    };

    using TextureMipMaps = std::array<std::pair<glm::uvec2, std::vector<glm::vec4>>, HapticMipMapLevels>;

    glm::dvec3 sample_force(float max_force, float surface_softness, float friction_scale, float surface_height,
                            FrictionMode friction_mode, unsigned int mip_map_level, const TextureMipMaps &tex_mip_maps,
                            const glm::dvec3 &pos, std::array<SimulationStepData, 2> last_steps = {},
                            const glm::dmat3 &haptic_mat = glm::dmat3{1.0});
}

#endif //MOLUMES_PHYSICS_H
