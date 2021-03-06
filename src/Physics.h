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
    template<typename T, std::size_t I>
    class SizedQueue {
    public:
        std::array<T, I> m_data;

        void push(T &&elem) {
            static_assert(0 < I);
            for (std::size_t i{0}; i + 1 < I; ++i)
                m_data[i] = std::move(m_data[i + 1]);
            m_data[I - 1] = std::forward<T>(elem);
        }

        auto &emplace() {
            push({});
            return m_data.back();
        }

        [[nodiscard]] auto &get() { return m_data; }

        [[nodiscard]] auto get() const { return m_data; }

        template<std::size_t N>
        [[nodiscard]] auto &get() {
            static_assert(N < I);
            return m_data[N];
        }

        template<std::size_t N>
        [[nodiscard]] auto get() const {
            static_assert(N < I);
            return m_data[N];
        }

        template<std::size_t N>
        [[nodiscard]] auto &get_from_back() { return get<I - N - 1>(); }

        template<std::size_t N>
        [[nodiscard]] auto get_from_back() const { return get<I - N - 1>(); }
    };

    enum FrictionMode : unsigned int {
        Disabled = 0,
        Uniform = 1
        // Directional
    };

    using TextureMipMaps = std::array<std::pair<glm::uvec2, std::vector<glm::vec4>>, HapticMipMapLevels>;

    /**
     * Utility object for physics simulation and force calculation.
     * Not only completely pure functions because it keeps an internal track of data from previous simulation steps.
     */
    class Physics {
    public:
        // Data structure to record previous simulation data
        struct SimulationStepData {
            std::chrono::high_resolution_clock::duration::rep delta_us; // time since last recording in microseconds
            glm::dvec3 pos; // Current position
            glm::dvec3 velocity; // Delta velocity from last frame (to current pos)
            glm::dvec3 normal_force{0.0, 0.0, 1.0};
            float surface_height{1.f}; // Height of pos relative to surface (initialize to 1 (above surface))
            glm::dvec3 sticktion_point;
            glm::dvec3 surface_pos;
            glm::dvec3 surface_velocity;
            struct Plane {
                glm::dvec3 normal;
                glm::dvec3 pos;
            };
            std::optional<Plane> intersection_plane;
        };

        struct NormalLevelResult {
            glm::dvec3 normal, soft_normal;
            float height;

            auto operator+=(const NormalLevelResult &rhs) {
                normal += rhs.normal;
                soft_normal += rhs.soft_normal;
                height += rhs.height;
                return *this;
            }
        };

    private:
        SizedQueue<SimulationStepData, 2> m_simulation_steps{};
        std::chrono::high_resolution_clock::time_point m_tp{std::chrono::high_resolution_clock::now()};

        [[nodiscard]] SimulationStepData &create_simulation_record(const glm::dvec3 &pos);

        void update_itnersecting(float surface_softness, const glm::dvec3 &pos,
                                 SimulationStepData &current_simulation_step,
                                 const glm::dvec3 &normal_force, float surface_height);

    public:
        glm::dvec3
        simulate_and_sample_force(double surface_force, float surface_softness, float surface_height_multiplier,
                                  unsigned int mip_map_level, const TextureMipMaps &tex_mip_maps, glm::dvec3 pos,
                                  std::optional<float> friction_scale = std::nullopt,
                                  std::optional<float> gravity_factor = std::nullopt,
                                  std::optional<unsigned int> surface_volume_mip_map_counts = std::nullopt,
                                  std::optional<float> sphere_kernel_radius = std::nullopt,
                                  bool volume_use_height_differences = false, float mip_map_scale_multiplier = 1.5f,
                                  bool pre_interpolative_normal = true, bool intersection_constraint = true);

        std::optional<NormalLevelResult>
        sample_normal(const glm::vec3 &coords, double surface_force, float surface_softness,
                      float surface_height_multiplier,
                      unsigned int mip_map_level, const TextureMipMaps &tex_mip_maps, const glm::dvec3 &pos,
                      const std::optional<unsigned int> &surface_volume_mip_map_counts,
                      const std::optional<float> &sphere_kernel_radius,
                      bool volume_use_height_differences, float mip_map_scale_multiplier, bool pre_interpolative_normal,
                      bool intersection_constraint);
    };

    std::vector<unsigned int>
    generate_enabled_mip_maps(unsigned int enabled_count = HapticMipMapLevels, unsigned int min_mip_map = 0);
}

#endif //MOLUMES_PHYSICS_H
