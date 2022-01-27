#include <globjects/globjects.h>
#include <globjects/State.h>

#include "GridSurfaceRenderer.h"

using namespace molumes;
using namespace globjects;

/**
 * @brief Helper function that creates a guard object which reverts back to it's original OpenGL state when it exits the scope.
 */
const auto stateGuard = []() {
    return std::shared_ptr<State>{State::currentState().release(), [](State *p) {
        Program::release();

        if (p != nullptr) {
            p->apply();
            delete p;
        }
    }};
};

GridSurfaceRenderer::GridSurfaceRenderer(Viewer *viewer) : Renderer(viewer) {
//    createShaderProgram("screen-debug", {
//            { GL_VERTEX_SHADER, "./res/crystal/screen-vs.glsl"},
//            { GL_FRAGMENT_SHADER, "./res/crystal/debug-fs.glsl"}
//    });
}

void GridSurfaceRenderer::display() {
    // Disable/remove this line on some Intel hardware were glbinding::State::apply() doesn't work:
    const auto state = stateGuard();

    
}
