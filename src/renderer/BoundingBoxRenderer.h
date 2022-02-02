#pragma once

#include "Renderer.h"
#include <memory>

namespace globjects {
    class File;
    class Program;
    class Buffer;
    class VertexArray;
}

namespace molumes {
    class Viewer;

    class BoundingBoxRenderer : public Renderer {
    public:
        BoundingBoxRenderer(Viewer *viewer);

        virtual void display();

        virtual std::list<globjects::File *> shaderFiles() const;

    private:

        std::unique_ptr<globjects::VertexArray> m_vao;
        std::unique_ptr<globjects::Buffer> m_vertices;
        std::unique_ptr<globjects::Buffer> m_indices;
        std::unique_ptr<globjects::Program> m_program;

        std::unique_ptr<globjects::File> m_vertexShaderSource = nullptr;
        std::unique_ptr<globjects::File> m_tesselationControlShaderSource = nullptr;
        std::unique_ptr<globjects::File> m_tesselationEvaluationShaderSource = nullptr;
        std::unique_ptr<globjects::File> m_geometryShaderSource = nullptr;
        std::unique_ptr<globjects::File> m_fragmentShaderSource = nullptr;
        gl::GLsizei m_size;
    };

}