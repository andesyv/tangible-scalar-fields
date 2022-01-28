#ifndef MOLUMES_HAPTICSINTERACTOR_H
#define MOLUMES_HAPTICSINTERACTOR_H

#include "Interactor.h"

/**
 * Enables interaction with haptic devices
 */

namespace molumes {
class HapticsInteractor : public Interactor {
public:
    HapticsInteractor(Viewer* viewer);
};
}


#endif //MOLUMES_HAPTICSINTERACTOR_H
