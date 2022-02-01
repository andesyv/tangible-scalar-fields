#ifndef MOLUMES_HAPTICSINTERACTOR_H
#define MOLUMES_HAPTICSINTERACTOR_H

#include <thread>

#include "Interactor.h"

/**
 * Enables interaction with haptic devices
 */

namespace molumes {
class HapticsInteractor : public Interactor {
private:
    std::thread m_thread;

public:
    explicit HapticsInteractor(Viewer* viewer);

};
}


#endif //MOLUMES_HAPTICSINTERACTOR_H
