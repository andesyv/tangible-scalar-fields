#include "BoundingBoxRenderer.h"
#include <globjects/base/File.h>
#include "../Viewer.h"

#include <glbinding/gl/gl.h>

#include <globjects/globjects.h>
#include <globjects/VertexAttributeBinding.h>
#include <globjects/NamedString.h>

using namespace molumes;
using namespace gl;
using namespace glm;
using namespace globjects;

BoundingBoxRenderer::BoundingBoxRenderer(Viewer *viewer)
        : Renderer(viewer), m_vao{std::make_unique<globjects::VertexArray>()},
          m_vertices{std::make_unique<globjects::Buffer>()}, m_indices{std::make_unique<globjects::Buffer>()},
          m_program{std::make_unique<globjects::Program>()} {
    static std::array<vec3, 8> vertices{{
                                                // front
                                                {-1.0, -1.0, 1.0},
                                                {1.0, -1.0, 1.0},
                                                {1.0, 1.0, 1.0},
                                                {-1.0, 1.0, 1.0},
                                                // back
                                                {-1.0, -1.0, -1.0},
                                                {1.0, -1.0, -1.0},
                                                {1.0, 1.0, -1.0},
                                                {-1.0, 1.0, -1.0}
                                        }};


    static std::array<std::array<GLushort, 4>, 6> indices{{
                                                                  // front
                                                                  {0, 1, 2, 3},
                                                                  // top
                                                                  {1, 5, 6, 2},
                                                                  // back
                                                                  {7, 6, 5, 4},
                                                                  // bottom
                                                                  {4, 0, 3, 7},
                                                                  // left
                                                                  {4, 5, 1, 0},
                                                                  // right
                                                                  {3, 2, 6, 7}
                                                          }};

    m_indices->setData(indices, GL_STATIC_DRAW);
    m_vertices->setData(vertices, GL_STATIC_DRAW);

    m_size = static_cast<GLsizei>(indices.size() * indices.front().size());
    m_vao->bindElementBuffer(m_indices.get());

    auto vertexBinding = m_vao->binding(0);
    vertexBinding->setAttribute(0);
    vertexBinding->setBuffer(m_vertices.get(), 0, sizeof(vec3));
    vertexBinding->setFormat(3, GL_FLOAT);
    m_vao->enable(0);

    m_vao->unbind();

    createShaderProgram("boundingbox", {
            { GL_VERTEX_SHADER, "./res/boundingbox/boundingbox-vs.glsl" },
            { GL_TESS_CONTROL_SHADER, "./res/boundingbox/boundingbox-tcs.glsl" },
            { GL_TESS_EVALUATION_SHADER, "./res/boundingbox/boundingbox-tes.glsl" },
            { GL_GEOMETRY_SHADER, "./res/boundingbox/boundingbox-gs.glsl" },
            { GL_FRAGMENT_SHADER, "./res/boundingbox/boundingbox-fs.glsl" }
    });
}

std::list<globjects::File *> BoundingBoxRenderer::shaderFiles() const {
    return std::list<globjects::File *>({
                                                m_vertexShaderSource.get(),
                                                m_tesselationControlShaderSource.get(),
                                                m_tesselationEvaluationShaderSource.get(),
                                                m_geometryShaderSource.get(),
                                                m_fragmentShaderSource.get()
                                        });
}

void BoundingBoxRenderer::display() {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    auto program = shaderProgram("boundingbox");
    program->setUniform("projection", viewer()->projectionTransform());
    program->setUniform("modelView", viewer()->viewTransform());

    program->use();

    m_vao->bind();
    glPatchParameteri(GL_PATCH_VERTICES, 4);
    m_vao->drawElements(GL_PATCHES, m_size, GL_UNSIGNED_SHORT, nullptr);
    m_vao->unbind();

    glDisable(GL_BLEND);
    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

    program->release();
}