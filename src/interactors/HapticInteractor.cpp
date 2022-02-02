#include "HapticInteractor.h"
#include "../Viewer.h"

#include <iostream>
#include <format>
#include <chrono>
#include <future>

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#include <imgui.h>

#ifdef DHD

#include <dhdc.h>

#endif

constexpr bool ENABLE_FORCE = false;

using namespace molumes;

#ifdef DHD

void haptic_loop(std::stop_token simulation_should_end, std::atomic<glm::vec3> &global_pos,
                 std::atomic<float> &interaction_bounds, std::promise<bool> &&setup_results,
                 Channel<std::vector<glm::vec4>> &&normal_tex_channel) {
    glm::dvec3 local_pos;
    bool force_enabled = false;
#ifndef NDEBUG
    constexpr auto DEBUG_INTERVAL = 1.0;
#endif
    double max_bound = 0.01;
    std::vector<glm::vec4> data{};

    // Initialize haptics device
    if (0 <= dhdOpen()) {
        std::cout << std::format("Haptic device detected: {}", dhdGetSystemName()) << std::endl;
        std::cout << std::format(
                "Device capabilities: base: {}, gripper: {}, wrist: {}, active gripper: {}, active wrist: {}",
                dhdHasBase(), dhdHasGripper(), dhdHasWrist(), dhdHasActiveGripper(), dhdHasActiveWrist()) << std::endl;
        setup_results.set_value(true);
    } else {
        // Early quit haptic loop by returning early out of the function
        setup_results.set_value(false);
        return;
    }

#ifndef NDEBUG
    auto last_debug_t = std::chrono::steady_clock::now();
#endif

    for (; !simulation_should_end.stop_requested();/*std::this_thread::sleep_for(std::chrono::milliseconds{1})*/) {
#ifndef NDEBUG
        const auto current_t = std::chrono::steady_clock::now();
        const auto debug_delta_time =
                static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(
                        current_t - last_debug_t).count()) * 0.001;
#endif
        // Query for position (actual rate of querying from hardware is controlled by underlying SDK)
        dhdGetPosition(&local_pos.x, &local_pos.y, &local_pos.z);

#ifndef NDEBUG
        const auto debug_frame = DEBUG_INTERVAL < debug_delta_time;
        if (debug_frame) {
            std::cout << std::format("Haptic local position: {}", glm::to_string(local_pos)) << std::endl;
            last_debug_t = current_t;
        }
#endif

        // Update max_bound for transformation calculation:
        max_bound = std::max({std::abs(local_pos.x), std::abs(local_pos.y), std::abs(local_pos.z), max_bound});

        // Transform to scene space:
        // Novint Falcon is in 10x10x10cm space = 0.1x0.1x0.1m = [-0.05, 0.05] m^3'
        // right = y, up = z, forward = -x
        const auto scale_mult =
                interaction_bounds.load(std::memory_order_relaxed) / max_bound; // [0, max_bound] -> [0, 10]
        local_pos = local_pos * scale_mult;

#ifndef NDEBUG
        if (debug_frame)
            std::cout << std::format("Haptic global position: {}", glm::to_string(local_pos)) << std::endl;
#endif

        global_pos.store(local_pos);

        // Attempt to fetch the last data sent through the channel.
        auto result = normal_tex_channel.try_get_last();
        if (result) {
            data = *result;
        }

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

HapticInteractor::HapticInteractor(Viewer *viewer, Channel<std::vector<glm::vec4>> &&normal_tex_channel) : Interactor(
        viewer) {
#ifdef DHD
    std::cout << std::format("Running dhd SDK version {}", dhdGetSDKVersionStr()) << std::endl;

    /// Possible handling of grip/rotation and other additional Haptic stuff here

    std::promise<bool> setup_results{};
    auto haptics_enabled = setup_results.get_future();
    m_thread = std::jthread{haptic_loop, std::ref(m_haptic_finger_pos), std::ref(m_interaction_bounds),
                            std::move(setup_results), std::move(normal_tex_channel)};
    m_haptic_enabled = haptics_enabled.get();
#endif
}

HapticInteractor::~HapticInteractor() {
    if (m_thread.joinable()) {
        m_thread.request_stop();
        m_thread.join();
    }
}

void HapticInteractor::display() {
    Interactor::display();

    if (ImGui::BeginMenu("Haptics")) {
        auto interaction_bounds = m_interaction_bounds.load();
        if (ImGui::SliderFloat("Interaction bounds", &interaction_bounds, 10.f, 3000.f))
            m_interaction_bounds.store(interaction_bounds);

        ImGui::EndMenu();
    }

    viewer()->m_haptic_pos = m_haptic_finger_pos.load();

    // Event such that haptic renderer can be disabled on runtime. For instance if the device loses connection
    static auto last_haptic_enabled = m_haptic_enabled;
    if (m_on_haptic_toggle && last_haptic_enabled != m_haptic_enabled) {
        m_on_haptic_toggle(m_haptic_enabled);
        last_haptic_enabled = m_haptic_enabled;
    }
}
