#pragma once

#include <memory>
#include <future>
#include <optional>
#include <tuple>

#include <glm/mat4x4.hpp>

#include "Renderer.h"
#include "../WorkerThread.h"
#include "../GeometryUtils.h"

namespace globjects {
    class VertexArray;

    class Buffer;

    class Sync;
}

namespace molumes {
    class Viewer;

    class CrystalRenderer : public Renderer {
    public:
        explicit CrystalRenderer(Viewer *viewer);

        void setEnabled(bool enabled) override;

        void display() override;

        auto getVertices() const { return m_vertices; }

        void fileLoaded(const std::string &) override;

#ifndef NDEBUG

        void reloadShaders() override;

#endif

    private:
        enum GeometryMode : int {
            Normal = 0,
            Mirror,
            Cut,
            Concave
        };

        std::unique_ptr<globjects::VertexArray> m_vao;
        // m_vertexBuffer is either m_computeBuffer or a smaller separate buffer subset
        std::shared_ptr<globjects::Buffer> m_computeBuffer, m_computeBuffer2, m_vertexBuffer; // Using shared_ptr to check if shared resource is same
        std::unique_ptr<globjects::Buffer> m_hullBuffer;
        std::unique_ptr<globjects::Buffer> m_maxValDiff;
        using WorkerThreadT = decltype(worker_manager_from_functions(getHexagonConvexHull, geometryPostProcessing));
        WorkerThreadT::ResultTypes m_workerResults;
        WorkerThreadT m_worker{};
        std::shared_ptr<bool> m_workerControlFlag; // Weird to have a pointer to a bool, but I'm only using this as a signal to the worker thread
        std::vector<glm::vec4> m_vertices;
        std::vector<unsigned int> m_vertexHull;

        bool m_wireframe = false;
        bool m_renderHull = true;
        bool m_tileNormalsEnabled = false;
        int m_renderStyleOption = 0;
        int m_topRegressionPlaneAlignment{1}, m_bottomRegressionPlaneAlignment{1};
        float m_tileScale = 1.0f;
        float m_tileHeight = 0.5f;
        float m_tileNormalsFactor = 1.0;
        float m_extrusionFactor = 0.2f;
        float m_valueThreshold = 0.f;
        float m_cutValue = 0.5f;
        float m_cutWidth = 0.1f;
        bool m_hexagonsUpdated = true;
        bool m_hexagonsSecondPartUpdated = false;
        gl::GLsizei m_drawingCount = 0;
        gl::GLsizei m_hullSize = 0;
        int m_geometryMode = 0;
        glm::mat4 m_modelMatrix{1.f};
        float m_minExtrusion = 0.f;

        void drawGUI(bool &bufferNeedsResize);

        [[nodiscard]] std::unique_ptr<globjects::Sync>
        generateBaseGeometry(std::shared_ptr<globjects::Texture> &&accumulateTexture,
                             std::shared_ptr<globjects::Buffer> &&accumulateMax,
                             const std::weak_ptr<globjects::Buffer> &tileNormalsRef, int tile_max_y, int count,
                             int num_cols, int num_rows, float tile_scale, glm::mat4 disp_mat);

        [[nodiscard]] std::unique_ptr<globjects::Sync>
        cullAndExtrude(std::shared_ptr<globjects::Texture> &&accumulateTexture,
                       const std::weak_ptr<globjects::Buffer> &tileNormalsRef,
                       int tile_max_y, int count, int num_cols, int num_rows, float tile_scale, glm::mat4 disp_mat);

        void resizeVertexBuffer(int hexCount);

        auto getGeometryMode() const { return static_cast<GeometryMode>(m_geometryMode); }

        static auto getDrawingCount(int count) { return count * 6 * 2 * 3; }

        auto getBufferPointCount(int count) const {
            return getDrawingCount(count) * 2 * (getGeometryMode() != Normal ? 2 : 1);
        }

        auto getBufferSize(int count) const {
            return static_cast<gl::GLsizeiptr>(getBufferPointCount(count) * sizeof(glm::vec4));
        } // * 2 to make room for extrusions

        glm::mat4 getModelMatrix(bool mirrorFlip = false) const;
    };
}