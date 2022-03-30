#pragma once

#include "Renderer.h"
#include <memory>

namespace globjects {
    class Buffer;
    class VertexArray;
}

namespace molumes {
    class Viewer;

    class AxisRenderer : public Renderer {
    public:
        AxisRenderer(Viewer *viewer);

        virtual void display();
    private:
        std::unique_ptr<globjects::VertexArray> m_vao;
        std::unique_ptr<globjects::Buffer> m_vbo;

    };

}