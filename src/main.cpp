#include <iostream>

#include <glbinding/Version.h>
#include <glbinding/Binding.h>
#include <glbinding/gl/enum.h>

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

    // Make an offscreen rendering context:
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    auto offscreen_window = glfwCreateWindow(1280, 720, "", nullptr, window);
    if (offscreen_window == nullptr) {
        globjects::critical() << "Failed to create offscreen rendering";
    }

    // Make context current
    glfwMakeContextCurrent(window);

    // Initialize globjects (internally initializes glbinding, and registers the current context)
    globjects::init([](const char *name) {
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


        constexpr long long US_PER_FRAME = 1000000u / 60u;
        constexpr long long US_ERROR_MARGIN_TIME = US_PER_FRAME / 3;

        glfwSwapInterval(0); // Set to 0 for "UNLIMITED FRAMES!!"

        Timer frame_timer{};

        // Main loop
        while (!glfwWindowShouldClose(window)) {
//        defaultState->apply();
            frame_timer.reset();
            glfwPollEvents();
            viewer->display();
            // Flush command queue but don't wait for finish just yet
            glFlush();
            //glFinish();
            // Allow offscreen rendering to do it's stuff while we wait for bufferswap:
            viewer->m_main_rendering_done.release();

            // https://gameprogrammingpatterns.com/game-loop.html
            const auto elapsed = frame_timer.elapsed<std::chrono::microseconds>();
            static constexpr auto MAX_TIME = US_PER_FRAME - US_ERROR_MARGIN_TIME;
            if (elapsed < MAX_TIME)
                std::this_thread::sleep_for(std::chrono::microseconds{MAX_TIME - elapsed});
            viewer->m_main_rendering_done.acquire();

            glfwMakeContextCurrent(window);

            // Finally wait and swap main buffers
            glfwSwapBuffers(window);
        }
    }

    // Destroy window(s)
    glfwDestroyWindow(offscreen_window);
    glfwDestroyWindow(window);

    // Properly shutdown GLFW
    glfwTerminate();

    return 0;
}
