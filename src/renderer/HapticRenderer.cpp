#include "HapticRenderer.h"
#include "../Utils.h"
#include "../Viewer.h"
#include "../DelegateUtils.h"
#include "../interactors/HapticInteractor.h"

#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>

#include <globjects/VertexArray.h>
#include <globjects/VertexAttributeBinding.h>
#include <globjects/Buffer.h>

#include <glm/gtc/matrix_transform.hpp>

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

    createShaderProgram("haptic-arrow", {
            { GL_VERTEX_SHADER, "./res/haptic/haptic-indicator-vs.glsl" },
            { GL_GEOMETRY_SHADER, "./res/haptic/haptic-indicator-arrow-gs.glsl" },
            { GL_FRAGMENT_SHADER, "./res/haptic/haptic-indicator-arrow-fs.glsl" }
    });

    subscribe(*viewer, &HapticInteractor::m_haptic_global_pos, [this](auto p){ m_haptic_pos = p; });
    subscribe(*viewer, &HapticInteractor::m_haptic_global_force, [this](auto f){
        auto f_l = glm::length(f);
        m_haptic_dir = 0.001f < f_l ? f * (1.f / f_l) : glm::vec3{0.f, 0.f, 1.f};
    });
    subscribe(*viewer, &HapticInteractor::m_ui_sphere_kernel_size, [this](auto r){ m_radius = r; });
}

void HapticRenderer::display() {
    auto state = stateGuard();

    if (ImGui::BeginMenu("Haptic Debug")) {
        ImGui::SliderFloat("Radius", &m_radius, 0.001f, 1.f);
        ImGui::SliderFloat("Arrow scale", &m_arrow_scale, 0.01f, 1.f);

        ImGui::EndMenu();
    }

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    const auto V = viewer()->viewTransform();
    const auto P = viewer()->projectionTransform();
    const auto PInv = glm::inverse(P);
    const auto MVP = P * V * glm::translate(glm::mat4{1.f}, m_haptic_pos);
    const auto haptic_dir = V * glm::vec4{m_haptic_dir, 0.f};

    {
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

    {
        const auto &shader = shaderProgram("haptic-arrow");
        if (!shader)
            return;

        shader->use();

        shader->setUniform("P", P);
        shader->setUniform("PInv", PInv);
        shader->setUniform("MVP", MVP);
        shader->setUniform("view_dir", glm::vec3{haptic_dir});
        shader->setUniform("scale", m_arrow_scale);

        m_vao->drawArrays(GL_POINTS, 0, 1);
    }

    VertexArray::unbind();
}

void HapticRenderer::setEnabled(bool enabled) {
    Renderer::setEnabled(enabled);

    if (enabled)
        viewer()->setPerspective(true);
}

