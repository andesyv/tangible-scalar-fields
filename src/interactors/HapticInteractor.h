#ifndef MOLUMES_HAPTICINTERACTOR_H
#define MOLUMES_HAPTICINTERACTOR_H

#include <thread>
#include <atomic>
#include <functional>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>

#include "Interactor.h"
#include "../Channel.h"
#include "../Constants.h"

namespace molumes {
/**
 * @brief This is the main class responsible for performing haptic rendering via a haptic force-feedback device.
 * Rendering via a haptic device is done by interacting with the IO device itself, which is why this is an Interactor
 * and not a Renderer.
 * The HapticInteractor is responsible for the renderloop with the haptic force-feedback device, which it does
 * by managing a separate thread. This means all communication from the rest of the program with the haptic device has
 * to go via atomics. Most of the actual physics calculations are delegated to the Physics class.
 */
    class HapticInteractor : public Interactor {
    public:
        struct HapticParams {
            std::atomic<glm::vec3> finger_pos, force;
            std::atomic<float> interaction_bounds{1.f}, surface_force{6.f}, surface_softness{0.031f},
                    sphere_kernel_radius{0.008f}, friction_scale{0.23f}, surface_height_multiplier{0.35f},
                    mip_map_scale_multiplier{1.3f};
            std::atomic<std::optional<float>> gravity_factor{std::nullopt};
            std::atomic<bool> enable_force{false}, monte_carlo_sampling{false}, surface_volume_mode{false},
                    volume_use_height_differences{false}, pre_interpolative_normals{true},
                    intersection_constraint{false}, enable_friction{true};
            std::atomic<unsigned int> mip_map_level{0}, input_space{0}, surface_volume_mip_map_count{
                    HapticMipMapLevels / 3};
            std::atomic<glm::dmat3> view_mat_inv, view_mat;
        };

        using MipMapLevel = std::pair<glm::uvec2, std::vector<glm::vec4>>;

    private:
        std::jthread m_thread;
        HapticParams m_params;
        bool m_haptic_enabled{false};

        static MipMapLevel generate_single_mipmap(glm::uvec2 tex_dims, std::vector<glm::vec4> tex_data);

        float m_ui_gravity_factor_value{2.f};

    public:
        std::function<void(bool)> m_on_haptic_toggle{};

        HapticInteractor() = default;

        explicit HapticInteractor(Viewer *viewer,
                                  ReaderChannel<std::array<MipMapLevel, HapticMipMapLevels>> &&normal_tex_channel);

        bool hapticEnabled() const { return m_haptic_enabled; }

        void keyEvent(int key, int scancode, int action, int mods) override;

        void display() override;

        template<std::size_t N = HapticMipMapLevels>
        static auto generateMipmaps(const glm::uvec2 &tex_dims, std::vector<glm::vec4> &&tex_data) {
            std::array<MipMapLevel, N> levels;
            levels.at(0) = std::make_pair(tex_dims, std::move(tex_data));
            for (glm::uint i = 1; i < N; ++i)
                levels.at(i) = generate_single_mipmap(levels.at(i - 1).first, levels.at(i - 1).second);
            return levels;
        }

        ~HapticInteractor() override;

        unsigned int m_mip_map_ui_level{0};
        std::vector<unsigned int> m_ui_surface_volume_enabled_mip_maps;
        int m_ui_surface_volume_mip_map_count{HapticMipMapLevels};
        glm::vec3 m_haptic_global_pos{}, m_haptic_global_force{};
        float m_ui_sphere_kernel_size, m_ui_surface_height_multiplier, m_ui_mip_map_scale_multiplier;
        bool m_ui_surface_volume_mode{false};
    };
}


#endif //MOLUMES_HAPTICINTERACTOR_H
