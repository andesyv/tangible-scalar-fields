#include "GridSurfaceRenderer.h"

#include <cassert>

#include <glbinding/gl/enum.h>
#include <glbinding/gl/types.h>
#include <glbinding/gl/bitfield.h>

#include <globjects/globjects.h>
#include <globjects/State.h>
#include <globjects/VertexAttributeBinding.h>

#include <glm/glm.hpp>

#include "../Viewer.h"
#include "globjects/Sync.h"

using namespace gl;
using namespace molumes;
using namespace globjects;

constexpr glm::uvec2 GRID_SIZE = {10u, 10u};
constexpr GLsizei VERTEX_COUNT = GRID_SIZE.x * GRID_SIZE.y * 6;
constexpr glm::uint TESSELATION = 16;

/**
 * @brief Helper function that creates a guard object which reverts back to it's original OpenGL state when it exits the scope.
 */
const auto stateGuard = []() {
    return std::shared_ptr<State>{State::currentState().release(), [](State *p) {
        Program::release();
        VertexArray::unbind();

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

    /*
     * Grid of quads: x * y * 6 vertices, (x-1)*(y-1)*2 triangles
     */
    m_grid = Buffer::create();
    m_grid->setStorage(sizeof(glm::vec3) * TESSELATION * TESSELATION * 4, nullptr, GL_MAP_READ_BIT);

    m_vao = VertexArray::create();

    // Init plane:
    m_planeBounds = Buffer::create();
    m_planeBounds->setData(std::to_array<std::pair<glm::vec3, glm::vec2>>({
                                                                                  {{1.f,  -1.f, 0.f}, {1.f, 0.f}},
                                                                                  {{1.f,  1.f,  0.f}, {1.f, 1.f}},
                                                                                  {{-1.f, -1.f, 0.f}, {0.f, 0.f}},
                                                                                  {{-1.f, 1.f,  0.f}, {0.f, 1.f}}
                                                                          }), GL_STATIC_DRAW);

    m_planeVAO = VertexArray::create();
    auto binding = m_planeVAO->binding(0);
    binding->setAttribute(0);
    binding->setBuffer(m_planeBounds.get(), 0, sizeof(glm::vec3) + sizeof(glm::vec2));
    binding->setFormat(3, GL_FLOAT);
    m_planeVAO->enable(0);
    binding = m_planeVAO->binding(1);
    binding->setAttribute(1);
    binding->setBuffer(m_planeBounds.get(), sizeof(glm::vec3), sizeof(glm::vec3) + sizeof(glm::vec2));
    binding->setFormat(2, GL_FLOAT);
    m_planeVAO->enable(1);

    m_screenSpacedBuffer = Buffer::create();
    m_screenSpacedBuffer->setData(std::to_array({
            glm::vec2{-1.f, -1.f},
            glm::vec2{1.f, -1.f},
            glm::vec2{1.f, 1.f},

            glm::vec2{1.f, 1.f},
            glm::vec2{-1.f, 1.f},
            glm::vec2{-1.f, -1.f}
    }), GL_STATIC_DRAW);

    m_screenSpacedVAO = VertexArray::create();
    binding = m_screenSpacedVAO->binding(0);
    binding->setAttribute(0);
    binding->setBuffer(m_screenSpacedBuffer.get(), 0, sizeof(glm::vec2));
    binding->setFormat(2, GL_FLOAT);
    m_screenSpacedVAO->enable(0);

    VertexArray::unbind();

    glPatchParameteri(GL_PATCH_VERTICES, 3);

    createShaderProgram("geometry", {
            {GL_VERTEX_SHADER,          "./res/grid/standard-plane-vs.glsl"},
            {GL_TESS_CONTROL_SHADER,    "./res/grid/normal-surface-tcs.glsl"},
            {GL_TESS_EVALUATION_SHADER, "./res/grid/normal-surface-tes.glsl"},
//            { GL_GEOMETRY_SHADER, ".res/grid/normal-surface-gs.glsl" },
            {GL_FRAGMENT_SHADER,        "./res/grid/normal-surface-fs.glsl"}
    });

    createShaderProgram("screen-debug", {
            {GL_VERTEX_SHADER,   "./res/crystal/screen-vs.glsl"},
            {GL_FRAGMENT_SHADER, "./res/crystal/debug-fs.glsl"}
    });
}

void GridSurfaceRenderer::display() {
    // Disable/remove this line on some Intel hardware were glbinding::State::apply() doesn't work:
    const auto state = stateGuard();

    auto resources = viewer()->m_sharedResources;
    const glm::mat4 MVP = viewer()->projectionTransform() * viewer()->viewTransform();
    static std::unique_ptr<Sync> syncObject = {};

//    /// ------------------- Geometry pass -------------------------------------------
//    {
//        const auto shader = shaderProgram("depth");
//        if (!shader)
//            return;
//
//
//    }

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    glPolygonMode(GL_FRONT, GL_FILL);
    glPolygonMode(GL_BACK, GL_LINE);
//    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    /// ------------------- Render pass -------------------------------------------
//    {
//        const auto shader = shaderProgram("geometry");
//        assert(shader);
//
////        auto binding = m_vao->binding(0);
////        binding->setAttribute(0);
////        binding->setBuffer(m_grid.get(), 0, sizeof(glm::vec4));
////        binding->setFormat(4, GL_FLOAT);
////        m_vao->enable(0);
//
//        shader->use();
//
//        m_grid->bindBase(GL_SHADER_STORAGE_BUFFER, 0);
//
//        shader->setUniform("MVP", MVP);
//        shader->setUniform("tesselation", TESSELATION);
//
//        glPatchParameteri(GL_PATCH_VERTICES, 4);
//        m_planeVAO->drawArrays(GL_PATCHES, 0, 4);
//
//        m_grid->unbind(GL_SHADER_STORAGE_BUFFER, 0);
//
//        glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
////        syncObject = std::move(Sync::fence(GL_SYNC_GPU_COMMANDS_COMPLETE));
////
////        if (m_GPU_vertices.empty() && syncObject) {
////            m_GPU_vertices
////        }
//    }

#ifndef NDEBUG
    {
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        const auto &shader = shaderProgram("screen-debug");
        if (shader && !resources.smoothNormalsTexture.expired()) {
            shader->use();

            auto smoothNormalTex = resources.smoothNormalsTexture.lock();
            smoothNormalTex->bindActive(0);

            m_screenSpacedVAO->bind();
            m_screenSpacedVAO->drawArrays(GL_TRIANGLES, 0, 6);

            VertexArray::unbind();

            smoothNormalTex->unbindActive(0);
        }
    }
#endif
}
