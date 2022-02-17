#include <iostream>
#include <numeric>
#include <algorithm>

#include <glbinding/Version.h>
#include <glbinding/Binding.h>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/bitfield.h>
#include <glbinding/ContextHandle.h>

#include <glbinding-aux/ContextInfo.h>
#include <glbinding-aux/ValidVersions.h>

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/gtx/transform.hpp>

#include <globjects/globjects.h>
#include <globjects/logging.h>
#include <globjects/State.h>
#include <globjects/base/File.h>
#include <globjects/NamedString.h>
#include <globjects/Sync.h>
#include <globjects/Query.h>

#include "Scene.h"
#include "CSV/Table.h"
#include "Viewer.h"
#include "interactors/Interactor.h"
#include "renderer/Renderer.h"
#include "Utils.h"

using namespace gl;
using namespace glm;
using namespace globjects;
using namespace molumes;

void error_callback(int errnum, const char *errmsg) {
    globjects::critical() << errnum << ": " << errmsg << std::endl;
}

int main(int argc, char *argv[]) {
    // Initialize GLFW
    if (!glfwInit())
        return 1;

    glfwSetErrorCallback(error_callback);

    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
    glfwWindowHint(GLFW_DOUBLEBUFFER, true);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 8);
#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

    // Create a context and, if valid, make it current
    GLFWwindow *window = glfwCreateWindow(1280, 720, "molumes", nullptr, nullptr);

    if (window == nullptr) {
        globjects::critical() << "Context creation failed - terminating execution.";

        glfwTerminate();
        return 1;
    }

    // Make context current
    glfwMakeContextCurrent(window);

    // Initialize globjects (internally initializes glbinding, and registers the current context)
    glbinding::ContextHandle context_handle = 0;
    globjects::init(context_handle, [](const char *name) {
        return glfwGetProcAddress(name);
    }, globjects::Shader::IncludeImplementation::Fallback);

    const std::string fileName = (argc > 1) ? std::string(argv[1]) : "./dat/0_10_sampled_testdata_10.000.csv";

#ifndef NDEBUG
    // Enable debug logging
    //	globjects::DebugMessage::enable();

    globjects::debug()
            << "OpenGL Version:  " << glbinding::aux::ContextInfo::version() << std::endl
            << "OpenGL Vendor:   " << glbinding::aux::ContextInfo::vendor() << std::endl
            << "OpenGL Renderer: " << glbinding::aux::ContextInfo::renderer() << std::endl;


    glEnable(GL_DEBUG_OUTPUT);
    glEnable(
            GL_DEBUG_OUTPUT_SYNCHRONOUS); // If you want to ensure the error happens exactly after the error on the same thread.
    glDebugMessageCallback(
            [](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message,
               const void *userParam) {
                using ESPair = std::pair<GLenum, std::string>;
                static const std::map<GLenum, std::string> SOURCES{
                        ESPair{GL_DEBUG_SOURCE_API, "GL_DEBUG_SOURCE_API"},
                        ESPair{GL_DEBUG_SOURCE_WINDOW_SYSTEM, "GL_DEBUG_SOURCE_WINDOW_SYSTEM"},
                        ESPair{GL_DEBUG_SOURCE_SHADER_COMPILER, "GL_DEBUG_SOURCE_SHADER_COMPILER"},
                        ESPair{GL_DEBUG_SOURCE_THIRD_PARTY, "GL_DEBUG_SOURCE_THIRD_PARTY"},
                        ESPair{GL_DEBUG_SOURCE_APPLICATION, "GL_DEBUG_SOURCE_APPLICATION"},
                        ESPair{GL_DEBUG_SOURCE_OTHER, "GL_DEBUG_SOURCE_OTHER"}
                };

                static const std::map<GLenum, std::string> TYPES{
                        ESPair{GL_DEBUG_TYPE_ERROR, "GL_DEBUG_TYPE_ERROR"},
                        ESPair{GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR, "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR"},
                        ESPair{GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR, "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR"},
                        ESPair{GL_DEBUG_TYPE_PORTABILITY, "GL_DEBUG_TYPE_PORTABILITY"},
                        ESPair{GL_DEBUG_TYPE_PERFORMANCE, "GL_DEBUG_TYPE_PERFORMANCE"},
                        ESPair{GL_DEBUG_TYPE_MARKER, "GL_DEBUG_TYPE_MARKER"},
                        ESPair{GL_DEBUG_TYPE_PUSH_GROUP, "GL_DEBUG_TYPE_PUSH_GROUP"},
                        ESPair{GL_DEBUG_TYPE_POP_GROUP, "GL_DEBUG_TYPE_POP_GROUP"},
                        ESPair{GL_DEBUG_TYPE_OTHER, "GL_DEBUG_TYPE_OTHER"}
                };

                static const std::map<GLenum, std::string> SEVERITIES{
                        ESPair{GL_DEBUG_SEVERITY_HIGH, "GL_DEBUG_SEVERITY_HIGH"},
                        ESPair{GL_DEBUG_SEVERITY_MEDIUM, "GL_DEBUG_SEVERITY_MEDIUM"},
                        ESPair{GL_DEBUG_SEVERITY_LOW, "GL_DEBUG_SEVERITY_LOW"},
                        ESPair{GL_DEBUG_SEVERITY_NOTIFICATION, "GL_DEBUG_SEVERITY_NOTIFICATION"}
                };

                const auto[sourceStr, typeStr, severityStr] = std::tie(SOURCES.at(source), TYPES.at(type),
                                                                       SEVERITIES.at(severity));
                std::cout << "GL_DEBUG: (source: " << sourceStr << ", type: " << typeStr << ", severity: "
                          << severityStr << ", message: " << message << std::endl;
                assert(severity != GL_DEBUG_SEVERITY_HIGH);
            }, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, false);
#endif

    auto defaultState = State::currentState();

    {
        auto scene = std::make_unique<Scene>();
        //scene->table()->load(fileName);
        auto viewer = std::make_unique<Viewer>(window, scene.get());
        if (!fileName.empty())
            viewer->openFile(fileName);

        // Scaling the model's bounding box to the canonical view volume
        vec3 boundingBoxSize = scene->table()->maximumBounds() - scene->table()->minimumBounds();
        float maximumSize = std::max({boundingBoxSize.x, boundingBoxSize.y, boundingBoxSize.z});
        mat4 modelTransform = scale(vec3(2.0f) / vec3(maximumSize));
        modelTransform =
                modelTransform * translate(-0.5f * (scene->table()->minimumBounds() + scene->table()->maximumBounds()));
        viewer->setModelTransform(modelTransform);


        constexpr long long NS_PER_FRAME =
                std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds{1}).count() / 60u;
        constexpr long long NS_ERROR_MARGIN_TIME = NS_PER_FRAME / 3;
        constexpr auto NS_MAX_FRAME_TIME = NS_PER_FRAME - NS_ERROR_MARGIN_TIME;
        constexpr bool MANUAL_VSYNC = true;

        glfwSwapInterval(0); // Set to 0 for "UNLIMITED FRAMES!!"

        unsigned int frame_count = 0;
        unsigned long long main_gpu_time_local_max{0}, main_gpu_time_max{NS_MAX_FRAME_TIME}, offload_rendering_time_max{1}, offload_rendering_time_local_max{0};

        std::unique_ptr<Query> main_rendering_query{Query::create()};
        std::unique_ptr<Sync> buffer_swapped_sync{};
        std::array<std::vector<std::unique_ptr<Query>>, 2> offload_rendering_queries{};
        unsigned int offload_rendering_query_index{0};
        auto calibration_time_point = std::chrono::high_resolution_clock::now();

        /**
         * Rendering loop is a bit complex:
         * 1. Fetch input
         * 2. Render each renderer while recording time of total operation
         * 3. Swap buffers (without vsync block)
         * 4. Wait for GPU to complete last operations of frame
         * 5. While / after waiting for screen refresh, perform background rendering / computational work
         * 6. Record time spent on main rendering loop + background rendering and use as estimations for next frame
         * 7. Manually sleep remaining time of frame-loop (to not perform more GPU work than needed)
         */
        while (!glfwWindowShouldClose(window)) {
//        defaultState->apply();
            auto frame_start = std::chrono::high_resolution_clock::now();
            glfwPollEvents();
            main_rendering_query->begin(GL_TIME_ELAPSED);
            viewer->display();
            main_rendering_query->end(GL_TIME_ELAPSED);

            // Finally wait and swap main buffers
            glfwSwapBuffers(window);

            // https://stackoverflow.com/questions/23612691/check-that-we-need-to-sleep-in-glfwswapbuffers-method
            buffer_swapped_sync = Sync::fence(GL_SYNC_GPU_COMMANDS_COMPLETE);
            bool offload_rendering_done = false;

            // Utilize free space before GPU has caught up from last frame to query new commands:
            for (uint i = 1; buffer_swapped_sync->clientWait(SyncObjectMask::GL_SYNC_FLUSH_COMMANDS_BIT, 0) ==
                             GL_TIMEOUT_EXPIRED; ++i) {
                if (!offload_rendering_done && (main_gpu_time_max + i * offload_rendering_time_max < NS_MAX_FRAME_TIME || viewer->m_forceOffloadRender)) {
                    viewer->m_forceOffloadRender = false;
                    offload_rendering_queries.at(offload_rendering_query_index).push_back(std::move(Query::create()));
                    auto &q = offload_rendering_queries.at(offload_rendering_query_index).back();
                    q->begin(GL_TIME_ELAPSED);
                    offload_rendering_done = viewer->offload_render();
                    q->end(GL_TIME_ELAPSED);
                }
            }

            // Here, the GPU is synced with CPU (last frame has happened):

            // Find max time of last frames offload rendering (for use in next frame:
            offload_rendering_query_index = !offload_rendering_query_index;
            offload_rendering_time_local_max = std::max(
                    std::reduce(offload_rendering_queries.at(offload_rendering_query_index).begin(),
                                offload_rendering_queries.at(offload_rendering_query_index).end(), GLuint64{0},
                                [](const auto &init, auto &q) {
                                    return std::max(init, q->get64(GL_QUERY_RESULT));
                                }), offload_rendering_time_local_max);
            offload_rendering_queries.at(offload_rendering_query_index).clear();
            offload_rendering_time_max = std::max(offload_rendering_time_max, offload_rendering_time_local_max);


#ifndef NDEBUG
            assert(main_rendering_query->resultAvailable());
#endif
            main_gpu_time_local_max = std::max(main_rendering_query->get64(GL_QUERY_RESULT), main_gpu_time_local_max);
            ++frame_count;

            // Every second, clear local maxima
            if (0 < std::chrono::duration_cast<std::chrono::seconds>(frame_start - calibration_time_point).count()) {
                calibration_time_point = frame_start;
                main_gpu_time_max = main_gpu_time_local_max;
                main_gpu_time_local_max = 0;

                // Make sure offload time also decreases its max every now and then (every 10th second)
                static unsigned long long offload_reset_timer{1};
                if (offload_reset_timer % 10 == 0) {
                    offload_rendering_time_max = 0 < offload_rendering_time_local_max ? offload_rendering_time_local_max : 1;
                    offload_rendering_time_local_max = 0;
                }
                ++offload_reset_timer;
            }

            // https://gameprogrammingpatterns.com/game-loop.html
            if constexpr (MANUAL_VSYNC)
                std::this_thread::sleep_until(frame_start + std::chrono::nanoseconds{NS_MAX_FRAME_TIME});
        }
    }

    // Destroy window
    glfwDestroyWindow(window);

    // Properly shutdown GLFW
    glfwTerminate();

    return 0;
}
