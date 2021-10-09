#include "CrystalRenderer.h"
#include "../Viewer.h"
#include "tileRenderer/Tile.h"

#include <array>
#include <format>
#include <algorithm>
#include <utility>

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

// https://en.cppreference.com/w/cpp/container/array/to_array
// template <typename T, std::size_t N, std::size_t... I>
// constexpr auto to_array_impl(T (&&args)[N], std::index_sequence<I...>) {
// 	return std::array<T, N>{ {args[I]...} }; /// Didn't know you could do this...
// }

// template <typename T, std::size_t N>
// constexpr auto to_array(T (&&args)[N]) {
// 	return to_array_impl(std::move(args), std::make_index_sequence<N>{});
// }

// template <typename T>
// concept Bindable = requires(T p) {
// 	p->bind();
// };

// template <Bindable T>
// class Guard {
// public:
// 	using ET = T::element_type;
// 	Guard(const T& ptr) : m_bindable{ptr.get()} {
// 		m_bindable->bind();
// 	}

// 	~Guard() {
// 		m_bindable->unbind();
// 	}

// private:
// 	ET* m_bindable;
// };

const auto stateGuard = []() {
    return std::shared_ptr<State>{State::currentState().release(), [](State *p) {
        if (p != nullptr) {
            p->apply();
            delete p;
        }
    }};
};

constexpr std::size_t MAX_HEXAGON_SIZE = 10000u;
constexpr auto VERTEXSTORAGEMASK = BufferStorageMask::GL_MAP_PERSISTENT_BIT | BufferStorageMask::GL_MAP_READ_BIT /*| BufferStorageMask::GL_MAP_WRITE_BIT | BufferStorageMask::GL_MAP_COHERENT_BIT*/;

// Waits for as little as it can and checks if the future is ready.
template <typename T>
bool isReady(const std::future<T>& f) {
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

    createShaderProgram("standard", {
            {GL_VERTEX_SHADER,   "./res/crystal/standard-vs.glsl"},
            {GL_FRAGMENT_SHADER, "./res/crystal/standard-fs.glsl"}
    });

}

void CrystalRenderer::setEnabled(bool enabled) {
    Renderer::setEnabled(enabled);

    if (enabled)
        viewer()->m_perspective = true;
}

void CrystalRenderer::display() {
    const auto state = stateGuard();

    if (ImGui::BeginMenu("Crystal")) {
        ImGui::Checkbox("Wireframe", &m_wireframe);
        if (ImGui::SliderFloat("Tile scale", &m_tileScale, 0.f, 4.f))
            m_hexagonsUpdated = true;
        if (ImGui::SliderFloat("Height", &m_tileHeight, 0.01f, 1.f))
            m_hexagonsUpdated = true;

        ImGui::EndMenu();
    }

    glEnable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, m_wireframe ? GL_LINE : GL_FILL);

    // Borrow resources from tileRenderer
    /// Borrowing like this prevents CrystalRenderer to gain ownership, but ensures resources are valid while drawing
    auto resources = viewer()->m_sharedResources;
    if (!resources)
        return;

    static int lastCount = 0;
    const int count = resources.tile.expired() ? 0 : resources.tile.lock()->numTiles;
    if (count < 1)
        return;


    if (lastCount != count) {
        m_hexagonsUpdated = true;
        lastCount = count;
        drawingCount = count * 6 * 2 * 3;
        // If count has changed, recreate and resize buffer. (expensive operation, so we only do this whenever the count changes)
        resizeVertexBuffer(count);
    }

    const auto num_cols = static_cast<int>(std::ceil(std::sqrt(count)));
    const auto num_rows = (count - 1) / num_cols + 1;
    const float scale = m_tileScale / static_cast<float>(num_cols);
    const float horizontal_space = scale * 1.5f;
    const float vertical_space = std::sqrt(3.f) * scale;
    auto accumulateTexture = resources.tileAccumulateTexture.lock();
    auto accumulateMax = resources.tileAccumulateMax.lock();
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
    const mat4 modelViewProjectionMatrix = viewer()->projectionTransform() *
                                           viewer()->viewTransform(); // Skipping model matrix as it's supplied another way anyway.
    std::unique_ptr<Sync> syncObject;

    // Calculate triangles:
    if (m_hexagonsUpdated) {
        const auto &shader = shaderProgram("triangles");
        if (!shader)
            return;

        const auto invocationSpace = std::max(static_cast<GLuint>(std::ceil(std::pow(count, 1.0 / 3.0))), 1u);

        m_vertexBuffer->bindBase(GL_SHADER_STORAGE_BUFFER, 0);
        accumulateTexture->bindActive(1);
        accumulateMax->bindBase(GL_SHADER_STORAGE_BUFFER, 2);

        shader->use();
        shader->setUniform("num_cols", num_cols);
        shader->setUniform("num_rows", num_rows);
        shader->setUniform("height", m_tileHeight);
        shader->setUniform("tile_scale", scale);
        shader->setUniform("disp_mat", model);
        shader->setUniform("POINT_COUNT", static_cast<GLuint>(count));

        glDispatchCompute(invocationSpace, invocationSpace, invocationSpace);

        accumulateMax->unbind(GL_SHADER_STORAGE_BUFFER, 2);
        accumulateTexture->unbindActive(1);
        m_vertexBuffer->unbind(GL_SHADER_STORAGE_BUFFER, 0);

        /** Returning data from GPU:
         * 1. Memory barrier
         * 2. Create and initiate fence-sync object
         * 3. Initiate rest of drawing commands for frame
         * 4. Wait for sync at end of frame (remember to glFlush)
         * 5. Map buffer, copy data.
         * 6. Create thread and pass data to thread
         * 6. Unmap buffer
         */

        // We are going to use the buffer to read from, but also to
        glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
        syncObject = std::move(Sync::fence(GL_SYNC_GPU_COMMANDS_COMPLETE));
        m_hexagonsUpdated = false;

//        std::promise<std::optional<std::vector<glm::vec4>>> tempPromise;
//        m_workerResult = std::move(tempPromise.get_future());
//        tempPromise.set_value(geometryPostProcessing(std::move(Sync::fence(GL_SYNC_GPU_COMMANDS_COMPLETE)), count, std::weak_ptr{m_vertexBuffer}));
    }

    // Render triangles:
    {
        const auto &shader = shaderProgram("standard");
        if (!shader || drawingCount < 1)
            return;

        shader->use();
        shader->setUniform("MVP", modelViewProjectionMatrix);

        m_vao->bind();

        const auto binding = m_vao->binding(0);
        binding->setAttribute(0);
        m_vertexBuffer->bind(GL_ARRAY_BUFFER);
        binding->setBuffer(m_vertexBuffer.get(), 0, sizeof(vec4));
        binding->setFormat(4, GL_FLOAT);
        m_vao->enable(0);

        m_vao->drawArrays(GL_TRIANGLES, 0, drawingCount);
    }

    globjects::Program::release();

    // Background memory management:
    if (syncObject) {
        const auto syncResult = syncObject->clientWait(GL_SYNC_FLUSH_COMMANDS_BIT, 10000);
        if (syncResult != GL_WAIT_FAILED && syncResult != GL_TIMEOUT_EXPIRED) {
            const auto vCount = count * 6 * 2 * 3;
            const auto memPtr = reinterpret_cast<vec4*>(m_vertexBuffer->map(GL_READ_ONLY));
            m_workerResult = std::move(std::async(std::launch::async, geometryPostProcessing, std::vector<vec4>{memPtr, memPtr + vCount}, m_vertexBuffer));
            assert(m_vertexBuffer->unmap());
        }
    }

    // If worker is done with doing its things, replace the buffer with new values:
    if (m_workerResult.valid() && isReady(m_workerResult)) {
        auto result = m_workerResult.get();
        if (result) {
            // Orphan that buffer!
            m_vertexBuffer = Buffer::create();
            m_vertexBuffer->setStorage( static_cast<GLsizeiptr>(result->size() * sizeof(vec4)), nullptr, VERTEXSTORAGEMASK);
            drawingCount = static_cast<int>(result->size());
        }
    }
}

uint CrystalRenderer::calculateTriangleCount(int hexCount) {
    /// Probably better calculation: total triangles = 6 * 2 * hexCount - grid_edges * 2
    return calculateEdgeCount(hexCount) * 2 + 6 * hexCount;
}

glm::uint CrystalRenderer::calculateEdgeCount(int hexCount) {
    const auto num_cols = static_cast<int>(std::ceil(std::sqrt(hexCount)));

    uint sum{0};
    for (int i{0}; i < hexCount; ++i) {
        // I've flipped the x and y-axis in the compute-shader, making the coordinates flipped.
        // This has also confused me to the point where I no longer know how this code works, but it just works. :)

        const int col = i % num_cols;
        const int row = (i / num_cols) * 2 - (col % 2);

        const auto center_l = 0 < col;
        const auto center_r = 0 < row;

        sum += uint{center_l} + uint{center_r} + uint{center_l && center_r};
    }

    return sum;
}

void CrystalRenderer::resizeVertexBuffer(int hexCount) {
    const auto vCount = hexCount * 6 * 2 * 3;
    // Make sure the buffer can be read from

    m_vertexBuffer = Buffer::create();
    /// Note: glBufferStorage only changes characteristics of how data is stored, so data itself is just as fast when doing glBufferData
    m_vertexBuffer->setStorage(vCount * static_cast<GLsizeiptr>(sizeof(vec4)), nullptr, VERTEXSTORAGEMASK);

    m_vertexBuffer->bind(GL_SHADER_STORAGE_BUFFER);
//    m_vertexBuffer->map(GL_READ_WRITE)
//    m_vertexDataPtr = PersistentStoragePtr<glm::vec4>{m_vertexBuffer.get()};
}

std::optional<std::vector<vec4>> CrystalRenderer::geometryPostProcessing(const std::vector<vec4>& vertices, const std::weak_ptr<Buffer>& bufferPtr) {
    // If we at this point don't have a buffer, it means it got recreated somewhere in the meantime.
    // In which case we don't need this thread anymore.
    if (bufferPtr.expired())
        return {};

    // Filter out empty vertices
    std::vector<vec4> data;
    data.reserve(vertices.size());
    std::copy_if(vertices.begin(), vertices.end(), std::back_inserter(data), [](const auto& v){ return 0.1f < v.w; });

    // Do stuff:..
    if (bufferPtr.expired())
        return {};

    return data;
//    // After everything is done, create new buffer with new vertex data and return it
//    std::shared_ptr<Buffer> newBuffer = Buffer::create();
//    newBuffer->setStorage(vCount * static_cast<GLsizeiptr>(sizeof(vec4)), data.data(), VERTEXSTORAGEMASK);
//    return std::make_pair(std::move(newBuffer), vCount);
}
