#include "HapticsInteractor.h"

#include <iostream>
#include <format>
#include <chrono>

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#ifdef DHD

#include <dhdc.h>

#endif

constexpr bool ENABLE_FORCE = false;

using namespace molumes;

#ifdef DHD

void haptic_loop(std::atomic_flag &simulation_should_end, std::atomic<glm::vec3> &global_pos) {
    glm::dvec3 local_pos;
    bool force_enabled = false;
    constexpr auto DEBUG_INTERVAL = 3.0;

    // Initialize haptics device
    if (0 <= dhdOpen()) {
        std::cout << std::format("Haptic device detected: {}", dhdGetSystemName()) << std::endl;
        std::cout << std::format(
                "Device capabilities: base: {}, gripper: {}, wrist: {}, active gripper: {}, active wrist: {}",
                dhdHasBase(), dhdHasGripper(), dhdHasWrist(), dhdHasActiveGripper(), dhdHasActiveWrist()) << std::endl;
    } else {
        // Early quit haptic loop by returning early out of the function
        return;
    }

    auto last_debug_t = std::chrono::steady_clock::now();

    for (; !simulation_should_end.test();/*std::this_thread::sleep_for(std::chrono::milliseconds{1})*/) {
        const auto current_t = std::chrono::steady_clock::now();
        const auto debug_delta_time =
                static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(current_t - last_debug_t).count()) * 0.001;
        // Query for position (actual rate of querying from hardware is controlled by underlying SDK)
        dhdGetPosition(&local_pos.x, &local_pos.y, &local_pos.z);

        if (DEBUG_INTERVAL < debug_delta_time) {
            std::cout << std::format("Haptic local position: {}", glm::to_string(local_pos)) << std::endl;
            last_debug_t = current_t;
        }

        // Transform to scene space:
        // Novint Falcon is in 10x10x10cm space = 0.1x0.1x0.1m = [-0.05, 0.05] m^3
        // right = y, up = z, forward = -x
        local_pos = glm::dvec3{local_pos.y, local_pos.z, -local_pos.x} * 10.0; // * 100.0;

        global_pos.store(local_pos);

        // Simulation stuff
        if constexpr (ENABLE_FORCE) {
            // Wait to apply force until we've arrived at a safe space
            if (!force_enabled) {
                if (false /* glm::length(force) < EPSILON */) {
                    dhdEnableForce(DHD_ON);
                    force_enabled = true;
                    std::cout << "Force enabled!" << std::endl;
                } else
                    continue;
            }

            // dhdSetForce()
        }
    }

    const auto op_result = dhdClose();
    if (op_result < 0)
        std::cout << std::format("Failed to close device (error code: {}, error: {})", op_result, dhdErrorGetLastStr())
                  << std::endl;
}

#endif // DHD

HapticsInteractor::HapticsInteractor(Viewer *viewer) : Interactor(viewer) {
#ifdef DHD
    std::cout << std::format("Running dhd SDK version {}", dhdGetSDKVersionStr()) << std::endl;

    /// Possible handling of grip/rotation and other additional Haptic stuff here

    m_thread = std::thread{haptic_loop, std::ref(m_haptics_done), std::ref(haptic_finger_pos)};
#endif
}

HapticsInteractor::~HapticsInteractor() {
    if (m_thread.joinable()) {
        m_haptics_done.test_and_set();
        m_thread.join();
    }
}
