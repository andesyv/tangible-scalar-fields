#include "AxisRenderer.h"
#include "../Viewer.h"
#include "../Utils.h"

#include <glbinding/gl/types.h>
#include <glbinding/gl/enum.h>

#include <globjects/Buffer.h>
#include <globjects/VertexArray.h>
#include <globjects/VertexAttributeBinding.h>

using namespace molumes;
using namespace globjects;
using namespace gl;

AxisRenderer::AxisRenderer(Viewer *viewer) : Renderer(viewer), m_vao{VertexArray::create()}, m_vbo{Buffer::create()} {
    // Pos (x,y,z), color (r,g,b)
    constexpr static GLfloat vertices[] = {
            0.f, 0.f, 0.f,      1.f, 0.f, 0.f,
            1.f, 0.f, 0.f,      1.f, 0.f, 0.f,
            0.f, 0.f, 0.f,      0.f, 1.f, 0.f,
            0.f, 1.f, 0.f,      0.f, 1.f, 0.f,
            0.f, 0.f, 0.f,      0.f, 0.f, 1.f,
            0.f, 0.f, 1.f,      0.f, 0.f, 1.f
    };

    m_vbo->setData(vertices, GL_STATIC_DRAW);

    auto binding = m_vao->binding(0);
    binding->setAttribute(0);
    binding->setBuffer(m_vbo.get(), 0, sizeof(GLfloat) * 6);
    binding->setFormat(3, GL_FLOAT);
    m_vao->enable(0);

    binding = m_vao->binding(1);
    binding->setAttribute(1);
    binding->setBuffer(m_vbo.get(), sizeof(GLfloat) * 3, sizeof(GLfloat) * 6);
    binding->setFormat(3, GL_FLOAT);
    m_vao->enable(1);

    VertexArray::unbind();

    createShaderProgram("axis", {
            { GL_VERTEX_SHADER, "./res/axis/axis-vs.glsl" },
            { GL_FRAGMENT_SHADER, "./res/axis/axis-fs.glsl" }
    });
}

void AxisRenderer::display() {
    auto state = stateGuard();

    const auto V = viewer()->viewTransform();
    const auto P = viewer()->projectionTransform();
    const auto MVP = P * V;

    const auto &shader = shaderProgram("axis");
    if (!shader)
        return;

    shader->use();
    shader->setUniform("MVP", MVP);
    m_vao->drawArrays(GL_LINES, 0, 6);
}