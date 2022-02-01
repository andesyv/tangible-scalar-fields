#include "HapticsInteractor.h"

#include <iostream>
#include <format>

#ifdef DHD
#include <dhdc.h>
#endif

using namespace molumes;

HapticsInteractor::HapticsInteractor(Viewer *viewer) : Interactor(viewer) {
#ifdef DHD
    std::cout << std::format("Running dhd SDK version {}", dhdGetSDKVersionStr()) << std::endl;
#endif
}