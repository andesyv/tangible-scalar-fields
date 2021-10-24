#pragma once

#include <memory>
#include <future>
#include <optional>
#include <tuple>

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

#ifndef NDEBUG
        void reloadShaders() override;
#endif

    private:
        std::unique_ptr<globjects::VertexArray> m_vao;
        // m_vertexBuffer is either m_computeBuffer or a smaller separate buffer subset
        std::shared_ptr<globjects::Buffer> m_computeBuffer, m_vertexBuffer; // Using shared_ptr to check if shared resource is same
        std::unique_ptr<globjects::Buffer> m_hullBuffer;
        using WorkerThreadT = decltype(worker_manager_from_functions(getHexagonConvexHull, geometryPostProcessing));
        WorkerThreadT::ResultTypes m_workerResults;
        WorkerThreadT m_worker{};
        std::shared_ptr<bool> m_workerControlFlag; // Weird to have a pointer to a bool, but I'm only using this as a signal to the worker thread
        std::vector<glm::vec4> m_vertices;
        std::vector<unsigned int> m_vertexHull;

        bool m_wireframe = false;
        bool m_renderHull = true;
        float m_tileScale = 1.0f;
        float m_tileHeight = 1.0f;
        float m_extrusionFactor = 0.5f;
        bool m_hexagonsUpdated = true;
        gl::GLsizei m_drawingCount = 0;
        gl::GLsizei m_hullSize = 0;

        static glm::uint calculateTriangleCount(int hexCount);
        static glm::uint calculateEdgeCount(int hexCount);
        void resizeVertexBuffer(int hexCount);
    };
}