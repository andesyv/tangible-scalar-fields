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

namespace molumes {
/**
 * Enables interaction with haptic devices
 */
    class HapticInteractor : public Interactor {
    public:
        struct HapticParams {
            std::atomic<glm::vec3> finger_pos;
            std::atomic<float> interaction_bounds{1.f};
            std::atomic<bool> enable_force{false};
            std::atomic<float> max_force{1.f};
            std::atomic<unsigned int> mip_map_level{0};
            std::atomic<float> surface_softness{0.015f};
            std::atomic<glm::vec3> force;
            std::atomic<bool> sphere_kernel{false};
            std::atomic<float> sphere_kernel_radius{0.01f};
        };

    private:
        std::jthread m_thread;
        HapticParams m_params;
        bool m_haptic_enabled{false};
        std::atomic<glm::mat4> m_view_mat;

    public:
        std::function<void(bool)> m_on_haptic_toggle{};

        HapticInteractor() = default;

        explicit HapticInteractor(Viewer *viewer,
                                  ReaderChannel<std::array<std::pair<glm::uvec2, std::vector<glm::vec4>>, 4>> &&normal_tex_channel);

        bool hapticEnabled() const { return m_haptic_enabled; }

        void keyEvent(int key, int scancode, int action, int mods) override;

        void display() override;

        static std::array<std::pair<glm::uvec2, std::vector<glm::vec4>>, 4>
        generateMipmaps(const glm::uvec2 &tex_dims, const std::vector<glm::vec4> &tex_data);

        ~HapticInteractor() override;

        unsigned int m_mip_map_ui_level{0};
        glm::vec3 m_haptic_global_pos{}, m_haptic_global_force{};
        float m_ui_sphere_kernel_size = 0.01f;
    };
}


#endif //MOLUMES_HAPTICINTERACTOR_H
