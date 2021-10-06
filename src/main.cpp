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
#include "Interactor.h"
#include "renderer/Renderer.h"

using namespace gl;
using namespace glm;
using namespace globjects;
using namespace molumes;

void error_callback(int errnum, const char *errmsg) {
    globjects::critical() << errnum << ": " << errmsg << std::endl;
}

typedef std::pair<GLenum, std::string> ESPair;
#define ESTR(x) ESPair{x, #x}

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
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

    // Create a context and, if valid, make it current
    GLFWwindow *window = glfwCreateWindow(1280, 720, "molumes", NULL, NULL);

    if (window == nullptr) {
        globjects::critical() << "Context creation failed - terminating execution.";

        glfwTerminate();
        return 1;
    }

    // Make context current
    glfwMakeContextCurrent(window);

    // Initialize globjects (internally initializes glbinding, and registers the current context)
    globjects::init([](const char *name) {
        return glfwGetProcAddress(name);
    });

    // Enable debug logging
    //	globjects::DebugMessage::enable();

    globjects::debug()
            << "OpenGL Version:  " << glbinding::aux::ContextInfo::version() << std::endl
            << "OpenGL Vendor:   " << glbinding::aux::ContextInfo::vendor() << std::endl
            << "OpenGL Renderer: " << glbinding::aux::ContextInfo::renderer() << std::endl;

    //std::string fileName = "./dat/6b0x.pdb";
    std::string fileName = "";

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(
            GL_DEBUG_OUTPUT_SYNCHRONOUS); // If you want to ensure the error happens exactly after the error on the same thread.
    glDebugMessageCallback(
            [](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message,
               const void *userParam) {
                static const std::map<GLenum, std::string> SOURCES{
                        ESTR(GL_DEBUG_SOURCE_API),
                        ESTR(GL_DEBUG_SOURCE_WINDOW_SYSTEM),
                        ESTR(GL_DEBUG_SOURCE_SHADER_COMPILER),
                        ESTR(GL_DEBUG_SOURCE_THIRD_PARTY),
                        ESTR(GL_DEBUG_SOURCE_APPLICATION),
                        ESTR(GL_DEBUG_SOURCE_OTHER)
                };

                static const std::map<GLenum, std::string> TYPES{
                        ESTR(GL_DEBUG_TYPE_ERROR),
                        ESTR(GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR),
                        ESTR(GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR),
                        ESTR(GL_DEBUG_TYPE_PORTABILITY),
                        ESTR(GL_DEBUG_TYPE_PERFORMANCE),
                        ESTR(GL_DEBUG_TYPE_MARKER),
                        ESTR(GL_DEBUG_TYPE_PUSH_GROUP),
                        ESTR(GL_DEBUG_TYPE_POP_GROUP),
                        ESTR(GL_DEBUG_TYPE_OTHER)
                };

                static const std::map<GLenum, std::string> SEVERITIES{
                        ESTR(GL_DEBUG_SEVERITY_HIGH),
                        ESTR(GL_DEBUG_SEVERITY_MEDIUM),
                        ESTR(GL_DEBUG_SEVERITY_LOW),
                        ESTR(GL_DEBUG_SEVERITY_NOTIFICATION)
                };

                const auto[sourceStr, typeStr, severityStr] = std::tie(SOURCES.at(source), TYPES.at(type),
                                                                       SEVERITIES.at(severity));
                if (severity != GL_DEBUG_SEVERITY_NOTIFICATION) {
                    std::cout << "GL_DEBUG: (source: " << sourceStr << ", type: " << typeStr << ", severity: "
                              << severityStr << ", message: " << message << std::endl;
//                    assert(severity != GL_DEBUG_SEVERITY_HIGH);
                }
            }, nullptr);

    if (argc > 1)
        fileName = std::string(argv[1]);

    auto defaultState = State::currentState();

    auto scene = std::make_unique<Scene>();
    //scene->table()->load(fileName);
    auto viewer = std::make_unique<Viewer>(window, scene.get());

    // Scaling the model's bounding box to the canonical view volume
    vec3 boundingBoxSize = scene->table()->maximumBounds() - scene->table()->minimumBounds();
    float maximumSize = std::max({boundingBoxSize.x, boundingBoxSize.y, boundingBoxSize.z});
    mat4 modelTransform = scale(vec3(2.0f) / vec3(maximumSize));
    modelTransform =
            modelTransform * translate(-0.5f * (scene->table()->minimumBounds() + scene->table()->maximumBounds()));
    viewer->setModelTransform(modelTransform);


    glfwSwapInterval(1); // Set to 0 for "UNLIMITED FRAMES!!"

    // Main loop
    while (!glfwWindowShouldClose(window)) {
//        defaultState->apply();
        glfwPollEvents();
        viewer->display();
        //glFinish();
        glfwSwapBuffers(window);
    }

    // Destroy window
    glfwDestroyWindow(window);

    // Properly shutdown GLFW
    glfwTerminate();

    return 0;
}
