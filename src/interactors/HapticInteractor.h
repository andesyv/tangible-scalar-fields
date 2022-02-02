#ifndef MOLUMES_HAPTICINTERACTOR_H
#define MOLUMES_HAPTICINTERACTOR_H

#include <thread>
#include <atomic>
#include <functional>

#include <glm/vec3.hpp>

#include "Interactor.h"

/**
 * Enables interaction with haptic devices
 */

namespace molumes {
class HapticInteractor : public Interactor {
private:
    std::jthread m_thread;
    std::atomic<glm::vec3> m_haptic_finger_pos;
    std::atomic<float> m_interaction_bounds{1000.f};
    bool m_haptic_enabled{false};

public:
    std::function<void(bool)> m_on_haptic_toggle{};

    explicit HapticInteractor(Viewer* viewer);

    bool hapticEnabled() const { return m_haptic_enabled; }

    void display() override;

    ~HapticInteractor() override;
};
}


#endif //MOLUMES_HAPTICINTERACTOR_H
