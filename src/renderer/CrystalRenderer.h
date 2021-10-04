#pragma once

#include <memory>
#include "Renderer.h"

namespace globjects {
    class VertexArray;

    class Buffer;
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
        std::unique_ptr<globjects::Buffer> m_dummyVertexBuffer;
        std::unique_ptr<globjects::Buffer> m_vertexBuffer;

        bool m_wireframe = false;
        float m_tileScale = 1.0f;
        float m_tileHeight = 1.0f;
        bool m_hexagonsUpdated = true;
    };
}