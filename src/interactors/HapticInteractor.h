#ifndef MOLUMES_HAPTICINTERACTOR_H
#define MOLUMES_HAPTICINTERACTOR_H

#include <thread>
#include <atomic>
#include <functional>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/vec2.hpp>

#include "Interactor.h"
#include "../Channel.h"

namespace molumes {
/**
 * Enables interaction with haptic devices
 */
class HapticInteractor : public Interactor {
private:
    std::jthread m_thread;
    std::atomic<glm::vec3> m_haptic_finger_pos;
    std::atomic<float> m_interaction_bounds{1.f};
    std::atomic<bool> m_enable_force{false};
    bool m_haptic_enabled{false};

public:
    std::function<void(bool)> m_on_haptic_toggle{};

    explicit HapticInteractor(Viewer* viewer, Channel<std::pair<glm::ivec2, std::vector<glm::vec4>>>&& normal_tex_channel);

    bool hapticEnabled() const { return m_haptic_enabled; }

    void keyEvent(int key, int scancode, int action, int mods) override;
    void display() override;

    ~HapticInteractor() override;
};
}


#endif //MOLUMES_HAPTICINTERACTOR_H
