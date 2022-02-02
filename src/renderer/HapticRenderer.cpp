#include "HapticRenderer.h"
#include "../Utils.h"
#include "../Viewer.h"

#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>

#include <globjects/VertexArray.h>
#include <globjects/VertexAttributeBinding.h>
#include <globjects/Buffer.h>

#include <imgui.h>

using namespace molumes;
using namespace gl;
using namespace globjects;

HapticRenderer::HapticRenderer(Viewer *viewer) : Renderer(viewer) {
    m_vao = VertexArray::create();
    m_point_buffer = Buffer::create();
    m_point_buffer->setData(std::array{glm::vec3{0.0}}, GL_STATIC_DRAW);

    auto binding = m_vao->binding(0);
    binding->setAttribute(0);
    binding->setBuffer(m_point_buffer.get(), 0, sizeof(glm::vec3));
    binding->setFormat(3, GL_FLOAT);
    m_vao->enable(0);

    VertexArray::unbind();

    createShaderProgram("haptic", {
            { GL_VERTEX_SHADER, "./res/haptic/haptic-indicator-vs.glsl" },
            { GL_GEOMETRY_SHADER, "./res/haptic/haptic-indicator-gs.glsl" },
            { GL_FRAGMENT_SHADER, "./res/haptic/haptic-indicator-fs.glsl" }
    });
}

void HapticRenderer::display() {
    auto state = stateGuard();

    if (ImGui::BeginMenu("Haptic Debug")) {
        ImGui::SliderFloat("Radius", &m_radius, 0.01f, 10.f);

        ImGui::EndMenu();
    }

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    const auto P = viewer()->projectionTransform();
    const auto PInv = glm::inverse(P);
    const auto MVP = P * viewer()->modelViewTransform();

    const auto &shader = shaderProgram("haptic");
    if (!shader)
        return;

    shader->use();

    shader->setUniform("P", P);
    shader->setUniform("MVP", MVP);
    shader->setUniform("radius", m_radius);
    shader->setUniform("PInv", PInv);

    m_vao->drawArrays(GL_POINTS, 0, 1);
}

