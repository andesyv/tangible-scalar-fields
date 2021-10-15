#include "CrystalRenderer.h"
#include "../Viewer.h"
#include "tileRenderer/Tile.h"

#include <array>
#include <format>
#include <algorithm>
#include <utility>
#include <iterator>
#include <deque>
#include <tuple>
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
constexpr auto VERTEXSTORAGEMASK = BufferStorageMask::GL_MAP_PERSISTENT_BIT |
                                   BufferStorageMask::GL_MAP_READ_BIT /*| BufferStorageMask::GL_MAP_WRITE_BIT | BufferStorageMask::GL_MAP_COHERENT_BIT*/;

// Waits for as little as it can and checks if the future is ready.
template<typename T>
bool isReady(const std::future<T> &f) {
    return f.wait_for(std::chrono::nanoseconds{0}) == std::future_status::ready;
}

template<typename Iter>
class cyclic_iterator {
public:
    using difference_type = typename Iter::difference_type;

private:
    const Iter _beg;
    const Iter _end;
    difference_type _rangeSize;

public:
    difference_type i = 0;

    cyclic_iterator(Iter range_begin, Iter range_end, const difference_type &offset)
            : _beg{std::move(range_begin)}, _end{std::move(range_end)}, i{offset},
              _rangeSize{std::distance(_beg, _end)} {}

    cyclic_iterator(const cyclic_iterator &) = default;

    auto operator++() {
        ++i;
        return *this;
    }

    auto operator--() {
        --i;
        return *this;
    }

    auto get_iterator() {
        return (i < 0 ? _end : _beg) + static_cast<difference_type>(i % _rangeSize);
    }

    auto operator*() {
        return *get_iterator();
    }

    auto operator+(const difference_type &diff) {
        return cyclic_iterator{_beg, _end, i + diff};
    }

    auto operator-(const difference_type &diff) {
        return *this + (-diff);
    }
};

template<std::forward_iterator It, typename F>
auto filter(It begin, It end, F &&predicate) {
    using T = decltype(*(It{}));
    std::vector<std::remove_cvref_t<T>> out;
    out.reserve(std::distance(begin, end));
    std::copy_if(begin, end, std::back_inserter(out), predicate);
    return out;
}

template<std::forward_iterator It, typename F>
auto map(It begin, It end, F &&predicate) {
    using T = typename It::value_type; // Only works on iterators with value_type member
    using K = decltype(predicate(T{}));
    std::vector<std::remove_cvref_t<K>> out;
    out.reserve(std::distance(begin, end));
    std::transform(begin, end, std::back_inserter(out), predicate);
    return out;
}

auto scalarCross2D(const dvec2 &a, const dvec2 &b) {
    const auto c = cross(dvec3{a, 0.0}, dvec3{b, 0.0});
    return c.z;
}

bool ccw(const dvec2 &a, const dvec2 &b, double epsilon = 0.001f) {
    return epsilon <= scalarCross2D(a, b);
}

double dotBetween(const dvec2 &a, const dvec2 &b) {
    // -0.25 * [1,-1] + 0.25 -> [0,0.5]
    // 0.25 * [-1,1] + 0.75 -> [0.5,1]
    return ccw(a, b) ? (-0.25 * dot(a, b) + 0.25) : (0.25 * dot(a, b) + 0.75);
}

//bool insideHull(const vec3& p, const std::vector)

std::optional<std::pair<std::vector<vec4>, std::vector<vec4>>>
geometryPostProcessing(const std::vector<vec4> &vertices, const std::weak_ptr<Buffer> &bufferPtr, float tileHeight) {
    constexpr double EPS = 0.000001;

    // If we at this point don't have a buffer, it means it got recreated somewhere in the meantime.
    // In which case we don't need this thread anymore.
    if (bufferPtr.expired() || vertices.size() < 2)
        return std::nullopt;

    // Filter out empty vertices
    auto data = filter(vertices.begin(), vertices.end(), [](const auto &v) { return EPS < v.w; });

    // TODO: Handle case with 0-1 size nonEmptyValues

    // Find the points which has a non-zero value of z (z height is hex-value, meaning empty ones are empty hexes)
    std::vector<std::pair<dvec2, uint>> nonEmptyValues;
    uint first = 0;
    dvec2 mi{std::numeric_limits<float>::max()}, ma{std::numeric_limits<float>::min()};
    nonEmptyValues.reserve(data.size());
    for (uint i = 0; i < data.size(); ++i) {
        const auto &v = data[i];
        const auto w = dvec2{v.x, v.y};
        mi = min(w, mi);
        ma = max(w, ma);

        if (-tileHeight < v.z) {
            nonEmptyValues.emplace_back(w, i);
            // Also find smallest (for graham scan later)
            const auto& oldSmallest = std::get<0>(nonEmptyValues[first]);
            if (w.y < oldSmallest.y || w.y < oldSmallest.y + EPS && w.x < oldSmallest.x) // y' < y && x' <= x
                first = static_cast<uint>(nonEmptyValues.size() - 1);
        }
    }
    const auto boundingCenter = 0.5 * mi + 0.5 * ma;

    // Make sure smallest is first
    if (1 < nonEmptyValues.size())
        std::swap(nonEmptyValues[0], nonEmptyValues[first]);
    // Set comparison direction, the start of the polar angle circle, to be the first point
    const auto compareDir = normalize(nonEmptyValues[0].first - boundingCenter);

    // NB: Only works on 0 < size()
    std::vector<std::tuple<dvec2, uint, double>> nonEmptyValuesPolarAngled;
    nonEmptyValuesPolarAngled.reserve(nonEmptyValues.size());
    for (const auto&[p, i]: nonEmptyValues) {
        if (nonEmptyValues[0].second == i) {
            nonEmptyValuesPolarAngled.emplace_back(p, i, 0.f);
        } else {
            const auto dir = p - boundingCenter;
            const auto dir_len = length(dir);
            if (EPS < dir_len) {
                const auto pa = dotBetween(compareDir, dir * (1. / dir_len));
                nonEmptyValuesPolarAngled.emplace_back(p, i, pa);
            }
        }
    }

    // Sort points on polar angle
    std::sort(nonEmptyValuesPolarAngled.begin(), nonEmptyValuesPolarAngled.end(), [](const auto &a, const auto &b) {
        return std::get<2>(a) < std::get<2>(b);
    });

    // Graham scan implementation:
    std::deque<std::tuple<dvec2, uint, double>> convexHullStack{};
    for (const auto&[p, i, polar_angle]: nonEmptyValuesPolarAngled) {
        // Skip empty/near-empty increments:
        if (!convexHullStack.empty()) {
            const auto last = std::get<0>(convexHullStack[0]);
            const auto b = p - last;

            if (length(b) < EPS)
                continue;
        }

        // While we have enough points in our stack...
        while (2 <= convexHullStack.size()) {
            const auto last = std::get<0>(convexHullStack[0]);
            const auto second_last = std::get<0>(convexHullStack[1]);
            auto a = last - second_last;
            auto b = p - last;

            // If the new point forms a clockwise turn with the last points in the stack,
            // pop the last point from the stack
            if (ccw(a, b, EPS))
                break;
            else
                convexHullStack.pop_front();
        }

        // Since points are sorted on polar coordinates and empty increments are skipped,
        // no point that arrives here should give an empty increment
        convexHullStack.emplace_front(p, i, polar_angle);
    }

    // My Graham scan implementation isn't perfect, so traverse hull once more and fix concave edges
    // TODO: Only need to fix start / end point
    // Last point could, but doesn't have to, be part of the convex hull. So we start on the one before.

//    const std::size_t startOffset{convexHullStack.size() / 2};
//    auto beg{convexHullStack.rbegin()}, end{convexHullStack.rend()};
//    std::vector<decltype(convexHullStack.rbegin())> copyRanges;
//    copyRanges.push_back(convexHullStack.rbegin());
//    cyclic_iterator it{convexHullStack.rbegin(), convexHullStack.rend(),
//                       static_cast<decltype(beg)::difference_type>(startOffset)};
//    for (std::size_t i{0}; i < convexHullStack.size(); ++i, ++it) {
//        auto next = it + 1;
//        auto last = it - 1;
//
//        const auto curr_p = std::get<0>(*it);
//        const auto last_p = std::get<0>(*last);
//        const auto a = normalize(curr_p - last_p);
//
//        for (uint j{0}; j < convexHullStack.size() - 1; ++j, ++next) {
//            const auto next_p = std::get<0>(*next);
//            /// Should be safe to normalize as we've discarded zero-length vectors previously
//            const auto b = normalize(next_p - curr_p);
//
//            if (ccw(a, b))
//                break;
//        }
//
//
//
//        // If we skipped some points, update range by "deleting" them.
//        auto offsetEnd = std::distance(beg, it.get_iterator());
//        auto offsetBeg = std::distance(beg, next.get_iterator());
//        if (1 < std::abs(offsetEnd - offsetBeg)) {
//            auto e{it.get_iterator()}, b{next.get_iterator()};
//            if (offsetEnd < offsetBeg) {
//                copyRanges.push_back(beg + offsetEnd);
//                copyRanges.push_back(beg + offsetBeg);
//            } else {
//                copyRanges.push_back(beg + offsetBeg);
//                copyRanges.push_back(beg + offsetEnd);
//            }
//        }
//    }
//
//    copyRanges.push_back(end);
//    std::vector<std::tuple<dvec2, uint, double>> hullList;
//    hullList.reserve(convexHullStack.size());
//    for (int i{0}; i < copyRanges.size(); i += 2)
//        hullList.insert(hullList.end(), copyRanges.at(i), copyRanges.at(i + 1));

    // Optimization idea: Since the hull is sorted by polar angles, we knot roughly where to find a hull edge based on its angle.
    // So we can use a divide and conquer search instead of searching through all of them. (ex. recursively)

    // Filter out all points outside of hull
    // Hull is in clockwise order, meaning any points inside the hull should make a clockwise turn in respect to the hull
    // Any point who doesn't make a clockwise turn is outside the hull

    const auto hullListConverted = map(convexHullStack.rbegin(), convexHullStack.rend(), [tileHeight](const auto &t) {
        const auto p = std::get<0>(t);
        return glm::vec4{p.x, p.y, -tileHeight, 1.f};
    });

    return bufferPtr.expired() ? std::nullopt : std::make_optional(std::make_pair(data, hullListConverted));
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

    createShaderProgram("hull", {
            {GL_VERTEX_SHADER,   "./res/crystal/hull-vs.glsl"},
            {GL_FRAGMENT_SHADER, "./res/crystal/hull-fs.glsl"}
    });
}

void CrystalRenderer::setEnabled(bool enabled) {
    Renderer::setEnabled(enabled);

    if (enabled)
        viewer()->m_perspective = true;
}

void CrystalRenderer::display() {
    const auto state = stateGuard();
    static float hullDegree = 1.f;

    if (ImGui::BeginMenu("Crystal")) {
        ImGui::Checkbox("Wireframe", &m_wireframe);
        if (ImGui::SliderFloat("Tile scale", &m_tileScale, 0.f, 4.f))
            m_hexagonsUpdated = true;
        if (ImGui::SliderFloat("Height", &m_tileHeight, 0.01f, 1.f))
            m_hexagonsUpdated = true;
        ImGui::SliderFloat("Hull degree", &hullDegree, 0.01f, 1.f);

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
        m_drawingCount = count * 6 * 2 * 3;
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

        m_computeBuffer->bindBase(GL_SHADER_STORAGE_BUFFER, 0);
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
        m_computeBuffer->unbind(GL_SHADER_STORAGE_BUFFER, 0);

        // Share resource with m_vertexBuffer, orphaning the old buffer
        m_vertexBuffer = m_computeBuffer;
        m_drawingCount = count * 6 * 2 * 3;

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
        if (shader && 0 < m_drawingCount) {
            shader->use();
            shader->setUniform("MVP", modelViewProjectionMatrix);

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
    {
        const auto &shader = shaderProgram("hull");
        if (shader && m_hullBuffer && 0 < m_hullSize) {
            shader->use();
            shader->setUniform("MVP", modelViewProjectionMatrix);

            m_vao->bind();

            const auto binding = m_vao->binding(0);
            binding->setAttribute(0);
            m_hullBuffer->bind(GL_ARRAY_BUFFER);
            binding->setBuffer(m_hullBuffer.get(), 0, sizeof(vec4));
            binding->setFormat(4, GL_FLOAT);
            m_vao->enable(0);

            m_vao->drawArrays(GL_LINE_STRIP, 0, static_cast<int>(m_hullSize * hullDegree));
        }
    }

    globjects::Program::release();

    // Background memory management:
    if (syncObject) {
        const auto syncResult = syncObject->clientWait(GL_SYNC_FLUSH_COMMANDS_BIT, 100000000);
        if (syncResult != GL_WAIT_FAILED && syncResult != GL_TIMEOUT_EXPIRED) {
            const auto vCount = count * 6 * 2 * 3;
            const auto memPtr = reinterpret_cast<vec4 *>(m_computeBuffer->mapRange(0, vCount *
                                                                                      static_cast<GLsizeiptr>(sizeof(vec4)),
                                                                                   GL_MAP_READ_BIT));
            if (memPtr != nullptr)
                m_workerResult = std::move(std::async(std::launch::async, geometryPostProcessing,
                                                      std::vector<vec4>{memPtr + 0, memPtr + vCount}, m_computeBuffer,
                                                      m_tileHeight));
            assert(m_computeBuffer->unmap());
        } else {
            std::cout << "Error: Sync Object was "
                      << (syncResult == GL_WAIT_FAILED ? "GL_WAIT_FAILED" : "GL_TIMEOUT_EXPIRED") << std::endl;
        }
    }

    // If worker is done with doing its things, replace the buffer with new values:
    if (m_workerResult.valid() && isReady(m_workerResult)) {
        auto result = m_workerResult.get();
        if (result) {
            const auto&[vertices, hull] = *result;
            // Create new buffer subset (unlinking m_computeBuffer)
            m_vertexBuffer = Buffer::create();
            m_vertexBuffer->setStorage(static_cast<GLsizeiptr>(vertices.size() * sizeof(vec4)), vertices.data(),
                                       VERTEXSTORAGEMASK);
            m_drawingCount = static_cast<int>(vertices.size());

            m_hullBuffer = Buffer::create();
            m_hullBuffer->setStorage(static_cast<GLsizeiptr>(hull.size() * sizeof(vec4)), hull.data(),
                                     VERTEXSTORAGEMASK);
            m_hullSize = static_cast<int>(hull.size());
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

    m_computeBuffer = Buffer::create();
    /// Note: glBufferStorage only changes characteristics of how data is stored, so data itself is just as fast when doing glBufferData
    m_computeBuffer->setStorage(vCount * static_cast<GLsizeiptr>(sizeof(vec4)), nullptr, VERTEXSTORAGEMASK);
    assert(m_computeBuffer->getParameter(GL_BUFFER_SIZE) ==
           vCount * static_cast<GLsizeiptr>(sizeof(vec4))); // Check that requested size == actual size

    m_computeBuffer->bind(GL_SHADER_STORAGE_BUFFER);
//    m_vertexBuffer->map(GL_READ_WRITE)
//    m_vertexDataPtr = PersistentStoragePtr<glm::vec4>{m_vertexBuffer.get()};
}
