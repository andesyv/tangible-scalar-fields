#include "CrystalRenderer.h"
#include "../Viewer.h"
#include "tileRenderer/Tile.h"

#include <array>
#include <iostream>

#include <glbinding/gl/enum.h>
#include <globjects/globjects.h>
#include <globjects/base/AbstractStringSource.h>
#include <globjects/base/File.h>
#include <globjects/State.h>
#include <globjects/VertexAttributeBinding.h>
#include <globjects/NamedString.h>
#include <globjects/Sync.h>
#include <glm/gtx/string_cast.hpp>
#include <imgui.h>
#include <glbinding/gl/bitfield.h>

using namespace molumes;
using namespace globjects;
using namespace glm;
using namespace gl;

const auto stateGuard = []() {
    return std::shared_ptr<State>{State::currentState().release(), [](State *p) {
        if (p != nullptr) {
            p->apply();
            delete p;
        }
    }};
};

constexpr std::size_t MAX_HEXAGON_SIZE = 10000u;
constexpr auto VERTEXSTORAGEMASK = BufferStorageMask::GL_MAP_PERSISTENT_BIT |
                                   BufferStorageMask::GL_MAP_READ_BIT /*| BufferStorageMask::GL_MAP_WRITE_BIT | BufferStorageMask::GL_MAP_COHERENT_BIT*/;

// Waits for as little as it can and checks if the future is ready.
template<typename T>
bool isReady(const std::future<T> &f) {
    return f.wait_for(std::chrono::nanoseconds{0}) == std::future_status::ready;
}

CrystalRenderer::CrystalRenderer(Viewer *viewer) : Renderer(viewer) {
    resizeVertexBuffer(MAX_HEXAGON_SIZE);

    m_vao = std::make_unique<VertexArray>(); /// Apparently exactly the same as VertexArray::create();

    createShaderProgram("crystal", {
            {GL_VERTEX_SHADER,   "./res/crystal/crystal-vs.glsl"},
            {GL_GEOMETRY_SHADER, "./res/crystal/crystal-gs.glsl"},
            {GL_FRAGMENT_SHADER, "./res/crystal/crystal-fs.glsl"}
    });

    createShaderProgram("triangles", {
            {GL_COMPUTE_SHADER, "./res/crystal/calculate-hexagons-cs.glsl"}
    });

    createShaderProgram("depth", {
            {GL_VERTEX_SHADER,   "./res/crystal/standard-vs.glsl"},
            {GL_FRAGMENT_SHADER, "./res/crystal/depth-fs.glsl"}
    });

    createShaderProgram("phong", {
            {GL_VERTEX_SHADER,   "./res/crystal/standard-vs.glsl"},
            {GL_GEOMETRY_SHADER, "./res/crystal/phong-gs.glsl"},
            {GL_FRAGMENT_SHADER, "./res/crystal/phong-fs.glsl"}
    });

    createShaderProgram("hull", {
            {GL_VERTEX_SHADER,   "./res/crystal/hull-vs.glsl"},
            {GL_FRAGMENT_SHADER, "./res/crystal/hull-fs.glsl"}
    });

    createShaderProgram("edge-extrusion", {
            {GL_COMPUTE_SHADER, "./res/crystal/cull-and-extrude-hexagons-cs.glsl"}
    });
}

void CrystalRenderer::setEnabled(bool enabled) {
    Renderer::setEnabled(enabled);

    if (enabled)
        viewer()->setPerspective(true);
}

void CrystalRenderer::display() {
    const auto state = stateGuard();

    auto resources = viewer()->m_sharedResources;
    constexpr std::array<const char*, 2> renderStyles{"depth", "phong"};

    if (ImGui::BeginMenu("Crystal")) {
        if (ImGui::CollapsingHeader("Preview", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Combo("Rendering style", &m_renderStyleOption, renderStyles.data(), static_cast<int>(renderStyles.size()));
            ImGui::Checkbox("Wireframe", &m_wireframe);
            ImGui::Checkbox("Render hull", &m_renderHull);
        }
        // Doesn't work unless you actually generate the normal plane from the 2D view
        if (ImGui::Checkbox("Align with regression plane", &m_tileNormalsEnabled))
            m_hexagonsUpdated = true;
        if (m_tileNormalsEnabled) {
            if (ImGui::SliderFloat("Regression plane factor", &m_tileNormalsFactor, 0.01f, 100.f))
                m_hexagonsUpdated = true;
        }
        if (ImGui::SliderFloat("Tile scale", &m_tileScale, 0.f, 4.f))
            m_hexagonsUpdated = true;
        if (ImGui::SliderFloat("Height", &m_tileHeight, 0.01f, 1.f))
            m_hexagonsUpdated = true;
        if (ImGui::SliderFloat("Extrusion", &m_extrusionFactor, 0.1f, 1.f))
            m_hexagonsSecondPartUpdated = true;

        ImGui::EndMenu();
    }

    glEnable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, m_wireframe ? GL_LINE : GL_FILL);

    // Borrow resources from tileRenderer
    /// Borrowing like this prevents CrystalRenderer to gain ownership, but ensures resources are valid while drawing
    if (!all_t_weak_ptr(std::make_tuple(resources.tile, resources.tileAccumulateTexture, resources.tileAccumulateMax)))
        return;

    auto tile = resources.tile.lock();
    static int lastCount = 0;
    const int count = tile->numTiles;
    if (count < 1)
        return;


    if (lastCount != count) {
        m_hexagonsUpdated = true;
        lastCount = count;
        m_drawingCount = count * 6 * 2 * 3;
        // If count has changed, recreate and resize buffer. (expensive operation, so we only do this whenever the count changes)
        resizeVertexBuffer(count);
    }

    const auto num_cols = static_cast<int>(std::ceil(std::sqrt(count)));
    const auto num_rows = (count - 1) / num_cols + 1;
    const float scale = m_tileScale / static_cast<float>(num_cols);
    const float horizontal_space = scale * 1.5f;
    const float vertical_space = std::sqrt(3.f) * scale;
    /* This looks a bit scary, but it's just doing whatever this line:
     * gl_Position = vec4((col - 0.5 * (num_cols-1)) * horizontal_space, vertical_space * 0.5 * (row - num_rows + (num_cols != 1 ? 1.5 : 1.0)), 0.0, 1.0);
     * was doing in the shader, which just moves the hex grid to be centered in the bounding box cube.
     * This makes the shader part much simpler to read.
     */
    const mat4 mTrans = translate(
            mat4{1.f},
            vec3{
                    -0.5f * static_cast<float>(num_cols - 1),
                    (num_cols != 1 ? 1.5f : 1.f) - static_cast<float>(num_rows),
                    0.f
            }
    );
    const mat4 mScale = glm::scale(
            mat4{1.f},
            vec3{horizontal_space, vertical_space * 0.5f, 1.f}
    );
    const mat4 model = mScale * mTrans;
    const mat4 viewProjectionMatrix = viewer()->projectionTransform() *
                                      viewer()->viewTransform(); // Skipping model matrix as it's supplied another way anyway.
    const mat4 inverseViewProjectionMatrix = inverse(viewProjectionMatrix);
    const mat4 lightMatrix = viewer()->lightTransform();
    const mat4 modelViewMatrix = viewer()->modelViewTransform();
    const mat4 inverseModelViewMatrix = inverse(modelViewMatrix);
    vec4 lightPos = lightMatrix * vec4(0., 0., 0., 1.0);
    lightPos /= lightPos.w;
//    vec4 viewPos = inverseViewProjectionMatrix * vec4(0.0f, 0.0f, -1.f, 1.0f);
//    viewPos /= viewPos.w;
    std::unique_ptr<Sync> syncObject;

    // Calculate triangles:
    if (m_hexagonsUpdated) {
        syncObject = generateBaseGeometry(std::move(resources.tileAccumulateTexture.lock()),
                                          std::move(resources.tileAccumulateMax.lock()),
                                          m_tileNormalsEnabled ? resources.tileNormalsBuffer : std::weak_ptr<Buffer>{},
                                          tile->m_tileMaxY, count, num_cols, num_rows,
                                          scale, model);
        std::get<0>(m_workerResults) = {};
        std::get<1>(m_workerResults) = {};
    }

    // Render triangles:
    {
        const auto &shader = shaderProgram(renderStyles.at(m_renderStyleOption));
        if (shader && 0 < m_drawingCount) {
            shader->use();
            shader->setUniform("MVP", viewProjectionMatrix);
            shader->setUniform("lightPos", vec3{lightPos});
//            shader->setUniform("viewPos", vec3{viewPos});

            m_vao->bind();

            const auto binding = m_vao->binding(0);
            binding->setAttribute(0);
            m_vertexBuffer->bind(GL_ARRAY_BUFFER);
            binding->setBuffer(m_vertexBuffer.get(), 0, sizeof(vec4));
            binding->setFormat(4, GL_FLOAT);
            m_vao->enable(0);

            m_vao->drawArrays(GL_TRIANGLES, 0, m_drawingCount);
        }
    }

    // Render convex hull
    if (m_renderHull) {
        const auto &shader = shaderProgram("hull");
        if (shader && m_hullBuffer && 0 < m_hullSize) {
            shader->use();
            shader->setUniform("MVP", viewProjectionMatrix);

            m_vao->bind();

            const auto binding = m_vao->binding(0);
            binding->setAttribute(0);
            m_hullBuffer->bind(GL_ARRAY_BUFFER);
            binding->setBuffer(m_hullBuffer.get(), 0, sizeof(vec4));
            binding->setFormat(4, GL_FLOAT);
            m_vao->enable(0);

            m_vao->drawArrays(GL_LINE_STRIP, 0, static_cast<int>(m_hullSize));
        }
    }
    // 1. Hull calculation:
    // Transfer data from GPU to background worker thread (and start worker thread)
    if (syncObject) {

        const auto syncResult = syncObject->clientWait(GL_SYNC_FLUSH_COMMANDS_BIT, 100000000);
        if (syncResult != GL_WAIT_FAILED && syncResult != GL_TIMEOUT_EXPIRED) {
            const auto vCount = count * 6 * 2 * 3;
            const auto memPtr = reinterpret_cast<vec4 *>(m_computeBuffer->mapRange(0, vCount *
                                                                                      static_cast<GLsizeiptr>(sizeof(vec4)),
                                                                                   GL_MAP_READ_BIT));
            if (memPtr != nullptr) {
                m_vertices = std::vector<vec4>{memPtr + 0, memPtr + vCount};
                std::get<0>(m_workerResults) = std::move(m_worker.queue_job<0>(getHexagonConvexHull, m_vertices,
                                                                               std::move(std::weak_ptr{
                                                                                       m_workerControlFlag}),
                                                                               m_tileHeight));
            }
            assert(m_computeBuffer->unmap());
            m_computeBuffer->copySubData(m_computeBuffer2.get(), static_cast<GLsizeiptr>(vCount * sizeof(vec4)));
        } else {
            std::cout << "Error: Sync Object was "
                      << (syncResult == GL_WAIT_FAILED ? "GL_WAIT_FAILED" : "GL_TIMEOUT_EXPIRED") << std::endl;
        }

        syncObject = {};
    }

    // If worker is done, run a new compute-shader call to remove outside hexes and extrude edges
    if (std::get<0>(m_workerResults).valid() && isReady(std::get<0>(m_workerResults))) {
        auto result = std::get<0>(m_workerResults).get();
        if (result) {
            m_vertexHull = *result;

            std::vector<glm::vec4> hullVertices;
            hullVertices.reserve(m_vertexHull.size());
            for (auto i: m_vertexHull)
                hullVertices.push_back(m_vertices.at(i));
            m_hullBuffer = Buffer::create();
            m_hullBuffer->setStorage(static_cast<GLsizeiptr>(hullVertices.size() * sizeof(vec4)), hullVertices.data(),
                                     VERTEXSTORAGEMASK);
            m_hullSize = static_cast<int>(hullVertices.size());

            m_hexagonsSecondPartUpdated = true;
        }
    }

    if (m_hexagonsSecondPartUpdated) {
        /// Edge extrusion compute shader drawcall
        syncObject = cullAndExtrude(m_tileNormalsEnabled ? resources.tileNormalsBuffer : std::weak_ptr<Buffer>{},
                                    tile->m_tileMaxY, count, num_cols, num_rows, scale, model);

        std::get<1>(m_workerResults) = {};
    }

    // Wait for edge extruding until final geometry cleanup:
    if (syncObject) {
        const auto syncResult = syncObject->clientWait(GL_SYNC_FLUSH_COMMANDS_BIT, 100000000);
        if (syncResult != GL_WAIT_FAILED && syncResult != GL_TIMEOUT_EXPIRED) {
            const auto vCount = count * 6 * 2 * 3 * 2;
            const auto memPtr = reinterpret_cast<vec4 *>(m_computeBuffer2->mapRange(0, vCount *
                                                                                      static_cast<GLsizeiptr>(sizeof(vec4)),
                                                                                   GL_MAP_READ_BIT));
            if (memPtr != nullptr)
                std::get<1>(m_workerResults) = std::move(
                        m_worker.queue_job<1>(geometryPostProcessing,
                                              std::vector<vec4>{memPtr + 0, memPtr + vCount},
                                              std::move(std::weak_ptr{m_workerControlFlag})));
            assert(m_computeBuffer2->unmap());
        } else {
            std::cout << "Error: Sync Object was "
                      << (syncResult == GL_WAIT_FAILED ? "GL_WAIT_FAILED" : "GL_TIMEOUT_EXPIRED") << std::endl;
        }
    }

    // 2. Final geometry processing
    if (std::get<1>(m_workerResults).valid() && isReady(std::get<1>(m_workerResults))) {
        auto result = std::get<1>(m_workerResults).get();
        if (result) {
            m_vertices = *result;
            // Create new buffer subset (unlinking m_computeBuffer)
            m_vertexBuffer = Buffer::create();
            m_vertexBuffer->setStorage(static_cast<GLsizeiptr>(m_vertices.size() * sizeof(vec4)), m_vertices.data(),
                                       VERTEXSTORAGEMASK);
            m_drawingCount = static_cast<int>(m_vertices.size());
        }
    }

    globjects::Program::release();
}

std::unique_ptr<Sync> CrystalRenderer::generateBaseGeometry(std::shared_ptr<globjects::Texture> &&accumulateTexture,
                                                            std::shared_ptr<globjects::Buffer> &&accumulateMax,
                                                            const std::weak_ptr<globjects::Buffer>& tileNormalsRef,
                                                            int tile_max_y, int count, int num_cols, int num_rows,
                                                            float tile_scale, glm::mat4 model) {
    const auto &shader = shaderProgram("triangles");
    if (!shader)
        return {};

    m_workerControlFlag = std::make_shared<bool>(true);

    const auto invocationSpace = std::max(static_cast<GLuint>(std::ceil(std::pow(count, 1.0 / 3.0))), 1u);
    const bool tileNormalsEnabled = !tileNormalsRef.expired();
    std::shared_ptr<Buffer> tileNormalsBuffer;

    m_computeBuffer2->clearData(GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr);
    m_computeBuffer->clearData(GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr);

    m_computeBuffer->bindBase(GL_SHADER_STORAGE_BUFFER, 0);
    accumulateTexture->bindActive(1);
    accumulateMax->bindBase(GL_SHADER_STORAGE_BUFFER, 2);
    if (tileNormalsEnabled) {
        tileNormalsBuffer = tileNormalsRef.lock();
        tileNormalsBuffer->bindBase(GL_SHADER_STORAGE_BUFFER, 3);
    }

    shader->use();
    shader->setUniform("num_cols", num_cols);
    shader->setUniform("num_rows", num_rows);
    shader->setUniform("height", m_tileHeight);
    shader->setUniform("tile_scale", tile_scale);
    shader->setUniform("disp_mat", model);
    shader->setUniform("POINT_COUNT", static_cast<GLuint>(count));
    shader->setUniform("maxTexCoordY", tile_max_y);
    shader->setUniform("tileNormalsEnabled", tileNormalsEnabled);
    shader->setUniform("tileNormalDisplacementFactor", m_tileNormalsFactor);

    glDispatchCompute(invocationSpace, invocationSpace, invocationSpace);

    if (tileNormalsEnabled)
        tileNormalsBuffer->unbind(GL_SHADER_STORAGE_BUFFER, 3);
    accumulateMax->unbind(GL_SHADER_STORAGE_BUFFER, 2);
    accumulateTexture->unbindActive(1);
    m_computeBuffer->unbind(GL_SHADER_STORAGE_BUFFER, 0);

    // Share resource with m_vertexBuffer, orphaning the old buffer
    m_vertexBuffer = m_computeBuffer;
    m_drawingCount = count * 6 * 2 * 3;

    // We are going to use the buffer to read from, but also to
    glMemoryBarrier(
            GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
    auto syncObject = std::move(Sync::fence(GL_SYNC_GPU_COMMANDS_COMPLETE));
    m_hexagonsUpdated = false;

    return std::move(syncObject);
}

std::unique_ptr<globjects::Sync>
CrystalRenderer::cullAndExtrude(const std::weak_ptr<globjects::Buffer> &tileNormalsRef,
                                int tile_max_y, int count, int num_cols,
                                int num_rows, float tile_scale, glm::mat4 model) {
    const auto &shader = shaderProgram("edge-extrusion");
    if (!shader)
        return {};

    const auto invocationSpace = std::max(static_cast<GLuint>(std::ceil(std::pow(count, 1.0 / 3.0))), 1u);
    const bool tileNormalsEnabled = !tileNormalsRef.expired();
    std::shared_ptr<Buffer> tileNormalsBuffer;

    m_computeBuffer2->bindBase(GL_SHADER_STORAGE_BUFFER, 0);
    m_hullBuffer->bindBase(GL_SHADER_STORAGE_BUFFER, 1);

    if (tileNormalsEnabled) {
        tileNormalsBuffer = tileNormalsRef.lock();
        tileNormalsBuffer->bindBase(GL_SHADER_STORAGE_BUFFER, 3);
    }

    shader->use();
    shader->setUniform("num_cols", num_cols);
    shader->setUniform("num_rows", num_rows);
    shader->setUniform("height", m_tileHeight);
    shader->setUniform("tile_scale", tile_scale);
    shader->setUniform("disp_mat", model);
    shader->setUniform("POINT_COUNT", static_cast<GLuint>(count));
    shader->setUniform("HULL_SIZE", static_cast<GLuint>(m_hullSize));
    shader->setUniform("extrude_factor", m_extrusionFactor);
    shader->setUniform("maxTexCoordY", tile_max_y);
    shader->setUniform("tileNormalsEnabled", tileNormalsEnabled);
    shader->setUniform("tileNormalDisplacementFactor", m_tileNormalsFactor);

    glDispatchCompute(invocationSpace, invocationSpace, invocationSpace);

    if (tileNormalsEnabled)
        tileNormalsBuffer->unbind(GL_SHADER_STORAGE_BUFFER, 3);
    m_hullBuffer->unbind(GL_SHADER_STORAGE_BUFFER, 1);
    m_computeBuffer2->unbind(GL_SHADER_STORAGE_BUFFER, 0);

    // We are going to use the buffer to read from, but also to
    m_vertexBuffer = m_computeBuffer2;
    m_drawingCount = count * 6 * 2 * 3;

    // We are going to use the buffer to read from, but also bind to it
    glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
    auto syncObject = std::move(Sync::fence(GL_SYNC_GPU_COMMANDS_COMPLETE));
    m_hexagonsSecondPartUpdated = false;

    return std::move(syncObject);
}

void CrystalRenderer::resizeVertexBuffer(int hexCount) {
    const auto vCount = hexCount * 6 * 2 * 3 * 2; // * 2 at the end there to make room for extrusions
    // Make sure the buffer can be read from

    m_computeBuffer = Buffer::create();
    m_computeBuffer2 = Buffer::create();
    /// Note: glBufferStorage only changes characteristics of how data is stored, so data itself is just as fast when doing glBufferData
    m_computeBuffer->setStorage(vCount * static_cast<GLsizeiptr>(sizeof(vec4)), nullptr, VERTEXSTORAGEMASK);
    m_computeBuffer2->setStorage(vCount * static_cast<GLsizeiptr>(sizeof(vec4)), nullptr, VERTEXSTORAGEMASK);
    assert(m_computeBuffer->getParameter(GL_BUFFER_SIZE) ==
           vCount * static_cast<GLsizeiptr>(sizeof(vec4))); // Check that requested size == actual size

    m_computeBuffer->bind(GL_SHADER_STORAGE_BUFFER);
}

#ifndef NDEBUG

void CrystalRenderer::reloadShaders() {
    Renderer::reloadShaders();

    m_hexagonsUpdated = true;
}

#endif

void CrystalRenderer::fileLoaded(const std::string &file) {
    Renderer::fileLoaded(file);

    m_hexagonsUpdated = true;
}
