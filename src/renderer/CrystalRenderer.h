#pragma once

#include <memory>
#include <future>
#include <optional>

#include "Renderer.h"

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
        std::shared_ptr<globjects::Buffer> m_vertexBuffer; // Using shared_ptr to check if shared resource is same
        std::future<std::optional<std::vector<glm::vec4>>> m_workerResult;

        bool m_wireframe = false;
        float m_tileScale = 1.0f;
        float m_tileHeight = 1.0f;
        bool m_hexagonsUpdated = true;
        gl::GLsizei drawingCount = 0;

        static glm::uint calculateTriangleCount(int hexCount);
        static glm::uint calculateEdgeCount(int hexCount);
        void resizeVertexBuffer(int hexCount);
        static std::optional<std::vector<glm::vec4>> geometryPostProcessing(const std::vector<glm::vec4>& vertices, const std::weak_ptr<globjects::Buffer>& bufferPtr);
    };
}