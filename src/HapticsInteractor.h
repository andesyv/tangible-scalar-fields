#ifndef MOLUMES_HAPTICSINTERACTOR_H
#define MOLUMES_HAPTICSINTERACTOR_H

#include <thread>
#include <atomic>

#include <glm/vec3.hpp>

#include "Interactor.h"

/**
 * Enables interaction with haptic devices
 */

namespace molumes {
class HapticsInteractor : public Interactor {
private:
    std::thread m_thread;
    std::atomic_flag m_haptics_done;
    std::atomic<glm::vec3> haptic_finger_pos;

public:
    explicit HapticsInteractor(Viewer* viewer);

    ~HapticsInteractor() override;
};
}


#endif //MOLUMES_HAPTICSINTERACTOR_H
