#ifndef MOLUMES_HAPTICINTERACTOR_H
#define MOLUMES_HAPTICINTERACTOR_H

#include <thread>
#include <atomic>

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
    float m_interaction_bounds_gui = 1000.f;

public:
    explicit HapticInteractor(Viewer* viewer);

    void display() override;

    ~HapticInteractor() override;
};
}


#endif //MOLUMES_HAPTICINTERACTOR_H
