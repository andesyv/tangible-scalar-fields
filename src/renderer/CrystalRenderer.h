#pragma once

#include <memory>
#include <future>
#include <optional>

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

    private:
        std::unique_ptr<globjects::VertexArray> m_vao;
        // m_vertexBuffer is either m_computeBuffer or a smaller separate buffer subset
        std::shared_ptr<globjects::Buffer> m_computeBuffer, m_vertexBuffer; // Using shared_ptr to check if shared resource is same
        std::unique_ptr<globjects::Buffer> m_hullBuffer;
        using WorkerThreadT = decltype(worker_manager_from_function(getHexagonConvexHull));
        WorkerThreadT::ResultT m_workerResult;
        WorkerThreadT m_worker{};
        std::shared_ptr<bool> m_workerControlFlag; // Weird to have a pointer to a bool, but I'm only using this as a signal to the worker thread
        std::vector<glm::vec4> m_vertices;

        bool m_wireframe = false;
        bool m_renderHull = true;
        float m_tileScale = 1.0f;
        float m_tileHeight = 1.0f;
        bool m_hexagonsUpdated = true;
        gl::GLsizei m_drawingCount = 0;
        gl::GLsizei m_hullSize = 0;

        static glm::uint calculateTriangleCount(int hexCount);
        static glm::uint calculateEdgeCount(int hexCount);
        void resizeVertexBuffer(int hexCount);
    };
}