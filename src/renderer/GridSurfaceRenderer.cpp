#include "GridSurfaceRenderer.h"
#include "../Utils.h"
#include "../Viewer.h"
#include "../DelegateUtils.h"
#include "../interactors/HapticInteractor.h"
#include "../Physics.h"
#include "tileRenderer/TileRenderer.h"

#include <cassert>
#include <format>

#include <glbinding/gl/enum.h>
#include <glbinding/gl/types.h>
#include <glbinding/gl/bitfield.h>

#include <globjects/globjects.h>
#include <globjects/State.h>
#include <globjects/VertexAttributeBinding.h>

// From forward declarations in tileRenderer.h:
#include <globjects/base/File.h>
#include <globjects/NamedString.h>
#include <globjects/base/StaticStringSource.h>
#include <globjects/Sync.h>

#include <imgui.h>

#include <glm/gtc/matrix_transform.hpp>


using namespace gl;
using namespace molumes;
using namespace globjects;

constexpr glm::uvec2 GRID_SIZE = {10u, 10u};
constexpr GLsizei VERTEX_COUNT = GRID_SIZE.x * GRID_SIZE.y * 6;
constexpr glm::uint TESSELATION = 64;
constexpr int PLANE_SUBDIVISION = 3;

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

// https://stackoverflow.com/questions/1505675/power-of-an-integer-in-c
template<unsigned int p>
int constexpr pow(const int x) {
    if constexpr (p == 0) return 1;
    if constexpr (p == 1) return x;

    int tmp = pow<p / 2>(x);
    if constexpr ((p % 2) == 0) { return tmp * tmp; }
    else { return x * tmp * tmp; }
}

template<typename T>
concept Insertable = requires(T a, T b) {
    a.insert(std::end(a), std::begin(b), std::end(b));
};

/// Easier to make a custom concatenation function than to use C++20's horrible range library
template<Insertable T, Insertable ... Ts>
constexpr auto vec_insert(T &container, const Ts &... rest) {
    auto end = std::end(container);
    ((end = container.insert(end, std::begin(rest), std::end(rest))), ...);
    return end;
}

/**
 * Disallow implicit conversion from lvalue to universal reference
 * Why C++ thought it was a good idea to have the rvalue sign be the same as the universal reference is beyond me.
 * Cause now you have to explicitly delete lvalue referenced template functions to disallow non-move semantics.
 */
template<typename T, typename ... Ts>
constexpr T vec_cat(T &container, const Ts &... rest) = delete;

template<typename T, typename ... Ts>
constexpr T vec_cat(T &&container, const Ts &... rest) {
    vec_insert(container, rest...);
    return std::move(container); // NOLINT(bugprone-move-forwarding-reference)
}

constexpr auto
subdivide_plane(const std::vector<std::pair<glm::vec3, glm::vec2>> &quad, unsigned int subdivisions = 1) {
    if (subdivisions == 1)
        return quad;

    using PointType = std::pair<glm::vec3, glm::vec2>;

//    glm::vec3 min_p{std::numeric_limits<float>::max()}, max_p{std::numeric_limits<float>::min()};
//    glm::vec2 min_uv{std::numeric_limits<float>::max()}, max_uv{std::numeric_limits<float>::min()};
//
//    for (const auto& [p, uv] : quad) {
//        min_p = glm::min(p, min_p);
//        max_p = glm::max(p, max_p);
//        min_uv = glm::min(uv, min_uv);
//        max_uv = glm::max(uv, max_uv);
//    }
    // Quad follows same pattern, so min is always the 3rd one and min is always the 2nd one
    const auto &min{quad.at(2)}, max{quad.at(1)};

    const auto diff_p = (max.first - min.first) * 0.5f;
    const auto diff_uv = (max.second - min.second) * 0.5f;

    // Divide quad into 4 smaller quads
    std::vector<PointType> smaller_quads;
    smaller_quads.reserve(16);
    for (auto z = 0u; z < 2u; ++z) {
        for (auto x = 0u; x < 2u; ++x) {
            smaller_quads.emplace_back(min.first + glm::vec3{x + 1, 0.f, z} * diff_p,
                                       min.second + glm::vec2{x + 1, z} * diff_uv);
            smaller_quads.emplace_back(min.first + glm::vec3{x + 1, 0.f, z + 1} * diff_p,
                                       min.second + glm::vec2{x + 1, z + 1} * diff_uv);
            smaller_quads.emplace_back(min.first + glm::vec3{x, 0.f, z} * diff_p,
                                       min.second + glm::vec2{x, z} * diff_uv);
            smaller_quads.emplace_back(min.first + glm::vec3{x, 0.f, z + 1} * diff_p,
                                       min.second + glm::vec2{x, z + 1} * diff_uv);
        }
    }

    const auto concatted_points = vec_cat(
            subdivide_plane(std::vector<PointType>{smaller_quads.begin(), smaller_quads.begin() + 4},
                            subdivisions - 1),
            subdivide_plane(std::vector<PointType>{smaller_quads.begin() + 4, smaller_quads.begin() + 8},
                            subdivisions - 1),
            subdivide_plane(std::vector<PointType>{smaller_quads.begin() + 8, smaller_quads.begin() + 12},
                            subdivisions - 1),
            subdivide_plane(std::vector<PointType>{smaller_quads.begin() + 12, smaller_quads.begin() + 16},
                            subdivisions - 1)
    );

    return std::vector<PointType>{concatted_points.begin(), concatted_points.end()};
}

template<std::size_t I>
constexpr auto create_subdivided_plane() {
    constexpr std::size_t count = pow<I>(4);
    std::array<std::pair<glm::vec3, glm::vec2>, count> vertices;
    auto vector_vertices = subdivide_plane(std::vector<std::pair<glm::vec3, glm::vec2>>{
            {{1.f,  0.f, 1.f},  {1.f, 0.f}},
            {{1.f,  0.f, -1.f}, {1.f, 1.f}},
            {{-1.f, 0.f, 1.f},  {0.f, 0.f}},
            {{-1.f, 0.f, -1.f}, {0.f, 1.f}}
    }, I);
    for (auto i{0u}; i < count; ++i)
        vertices.at(i) = vector_vertices.at(i);
    return vertices;
}

GridSurfaceRenderer::GridSurfaceRenderer(Viewer *viewer) : Renderer(viewer) {
    constexpr auto plane_vertices = create_subdivided_plane<PLANE_SUBDIVISION>();

    // Init plane:
    m_planeBounds = Buffer::create();
    m_planeBounds->setData(plane_vertices, GL_STATIC_DRAW);

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
            {GL_FRAGMENT_SHADER,        "./res/grid/normal-surface-fs.glsl"}
    });

    m_surface_volume_enabled_mip_maps = generate_enabled_mip_maps(HapticMipMapLevels);

    subscribe(*viewer, &HapticInteractor::m_mip_map_ui_level, [this](unsigned int level) { m_mip_map_level = level; });
    subscribe(*viewer, &HapticInteractor::m_ui_surface_height_multiplier, [this](float m) { m_height_multiplier = m; });
    subscribe(*viewer, &HapticInteractor::m_ui_surface_volume_mode,
              [this](bool b) { m_surface_volume_mode = b; });
    subscribe(*viewer, &HapticInteractor::m_ui_surface_volume_enabled_mip_maps,
              [this](auto mips) { m_surface_volume_enabled_mip_maps = std::move(mips); });
    subscribe(*viewer, &TileRenderer::m_debug_heightmap, [this](auto b) { m_heightfield = b; });
}

void GridSurfaceRenderer::display() {
    // Disable/remove this line on some Intel hardware were glbinding::State::apply() doesn't work:
    const auto state = stateGuard();

    static glm::uint tesselation = TESSELATION;
    if (ImGui::BeginMenu("Grid plane")) {
        auto ui_tess = static_cast<int>(tesselation);
        // Apparently, most hardware has their max tesselation level set to 64. Not a lot for a single plane, but a lot for individual triangles.
        if (ImGui::DragInt("Tesselation", &ui_tess, 1.f, 1, 64))
            tesselation = static_cast<glm::uint>(ui_tess);

        ImGui::EndMenu();
    }

    auto resources = viewer()->m_sharedResources;
    const glm::mat4 MVP = viewer()->projectionTransform() * viewer()->viewTransform();

    if (resources.smoothNormalsTexture.expired())
        return;
    auto smoothNormalTex = resources.smoothNormalsTexture.lock();

    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    if (m_surface_volume_mode) {
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    glPolygonMode(GL_FRONT, GL_FILL);
    glPolygonMode(GL_BACK, m_surface_volume_mode ? GL_FILL : GL_LINE);

    /// ------------------- Render pass -------------------------------------------
    {
        const auto shader = shaderProgram("geometry");
        assert(shader);

        shader->use();

        BindActiveGuard _g{smoothNormalTex, 0};

        constexpr auto vertex_count = static_cast<GLsizei>(pow<PLANE_SUBDIVISION>(4));
        glPatchParameteri(GL_PATCH_VERTICES, 4);

        shader->setUniform("tesselation", tesselation);
        shader->setUniform("surface_height", m_height_multiplier);
        shader->setUniform("heightfield", m_heightfield);

        if (m_surface_volume_mode) {
            struct DrawParams {
                glm::mat4 mvp;
                float mip_map_level, opacity, hue_shift;
            };

            std::map<float, DrawParams, std::greater<>> drawCalls;


            const auto inv_v_mat = glm::inverse(viewer()->viewTransform());
            const auto camera_pos = glm::vec3{inv_v_mat * glm::vec4{0.f, 0.f, 0.f, 1.f}};

            for (int i = 1; i < m_surface_volume_enabled_mip_maps.size(); ++i) {
                const auto t = static_cast<float>(i) / static_cast<float>(m_surface_volume_enabled_mip_maps.size() - 1);
                const auto level = static_cast<float>(m_surface_volume_enabled_mip_maps.at(i));

                const auto pos = glm::vec3{0.f, std::lerp(-0.25f, 0.25f, t), 0.f};
                const auto mvp = glm::translate(MVP, pos);

                // Transparancy sorting done using a map sorted on distance to camera
                drawCalls.emplace(glm::length(pos - camera_pos), DrawParams{mvp, level, 0.4f, 1.f - t});
            }

            for (const auto&[key, drawcall]: drawCalls) {
                shader->setUniform("MVP", drawcall.mvp);
                shader->setUniform("mip_map_level", drawcall.mip_map_level);
                shader->setUniform("hue_shift", drawcall.hue_shift);
                shader->setUniform("opacity", drawcall.opacity);

                m_planeVAO->drawArrays(GL_PATCHES, 0, vertex_count);
            }
        }

        shader->setUniform("MVP", m_surface_volume_mode ? glm::translate(MVP, glm::vec3{0.f, -0.25f, 0.f}) : MVP);
        shader->setUniform("mip_map_level", static_cast<float>(m_mip_map_level));
        shader->setUniform("hue_shift", 1.f);
        shader->setUniform("opacity", 1.f);

        m_planeVAO->drawArrays(GL_PATCHES, 0, vertex_count);

        glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
    }
}

void GridSurfaceRenderer::setEnabled(bool enabled) {
    Renderer::setEnabled(enabled);

    if (enabled)
        viewer()->setPerspective(true);
}