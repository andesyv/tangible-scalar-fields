#include "CrystalRenderer.h"
#include "../Viewer.h"

#include <iostream>
#include <array>
// #include <glbinding/gl/gl.h>
#include <glbinding/gl/enum.h>
// #include <glbinding/gl/functions.h>
#include <globjects/VertexArray.h>
#include <globjects/State.h>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

using namespace molumes;
using namespace globjects;
using namespace glm;
using namespace gl;

// https://en.cppreference.com/w/cpp/container/array/to_array
// template <typename T, std::size_t N, std::size_t... I>
// constexpr auto to_array_impl(T (&&args)[N], std::index_sequence<I...>) {
// 	return std::array<T, N>{ {args[I]...} }; /// Didn't know you could do this...
// }

// template <typename T, std::size_t N>
// constexpr auto to_array(T (&&args)[N]) {
// 	return to_array_impl(std::move(args), std::make_index_sequence<N>{});
// }

// template <typename T>
// concept Bindable = requires(T p) {
// 	p->bind();
// };

// template <Bindable T>
// class Guard {
// public:
// 	using ET = T::element_type;
// 	Guard(const T& ptr) : m_bindable{ptr.get()} {
// 		m_bindable->bind();
// 	}

// 	~Guard() {
// 		m_bindable->unbind();
// 	}

// private:
// 	ET* m_bindable;
// };

//constexpr auto CUBEVERTICES = std::to_array({
//		-1.0f,-1.0f,-1.0f,
//		-1.0f,-1.0f, 1.0f,
//		-1.0f, 1.0f, 1.0f,
//		1.0f, 1.0f,-1.0f,
//		-1.0f,-1.0f,-1.0f,
//		-1.0f, 1.0f,-1.0f,
//		1.0f,-1.0f, 1.0f,
//		-1.0f,-1.0f,-1.0f,
//		1.0f,-1.0f,-1.0f,
//		1.0f, 1.0f,-1.0f,
//		1.0f,-1.0f,-1.0f,
//		-1.0f,-1.0f,-1.0f,
//		-1.0f,-1.0f,-1.0f,
//		-1.0f, 1.0f, 1.0f,
//		-1.0f, 1.0f,-1.0f,
//		1.0f,-1.0f, 1.0f,
//		-1.0f,-1.0f, 1.0f,
//		-1.0f,-1.0f,-1.0f,
//		-1.0f, 1.0f, 1.0f,
//		-1.0f,-1.0f, 1.0f,
//		1.0f,-1.0f, 1.0f,
//		1.0f, 1.0f, 1.0f,
//		1.0f,-1.0f,-1.0f,
//		1.0f, 1.0f,-1.0f,
//		1.0f,-1.0f,-1.0f,
//		1.0f, 1.0f, 1.0f,
//		1.0f,-1.0f, 1.0f,
//		1.0f, 1.0f, 1.0f,
//		1.0f, 1.0f,-1.0f,
//		-1.0f, 1.0f,-1.0f,
//		1.0f, 1.0f, 1.0f,
//		-1.0f, 1.0f,-1.0f,
//		-1.0f, 1.0f, 1.0f,
//		1.0f, 1.0f, 1.0f,
//		-1.0f, 1.0f, 1.0f,
//		1.0f,-1.0f, 1.0f});

constexpr auto CENTERVERTICES = std::to_array({
                                                      0.f, 0.f, 0.f
                                              });

CrystalRenderer::CrystalRenderer(Viewer *viewer) : Renderer(viewer) {
    m_vao = std::make_unique<VertexArray>(); /// Apparantly exactly the same as VertexArray::create();
    m_vertexBuffer = Buffer::create();
    m_vertexBuffer->setData(CENTERVERTICES, GL_STATIC_DRAW);

    const auto binding = m_vao->binding(0);
    binding->setAttribute(0);
    binding->setBuffer(m_vertexBuffer.get(), 0, sizeof(vec3));
    binding->setFormat(3, GL_FLOAT);
    m_vao->enable(0);

    createShaderProgram("crystal", {
            {GL_VERTEX_SHADER,   "./res/crystal/crystal-vs.glsl"},
            {GL_GEOMETRY_SHADER, "./res/crystal/crystal-gs.glsl"},
            {GL_FRAGMENT_SHADER, "./res/crystal/crystal-fs.glsl"}
    });
}

void CrystalRenderer::setEnabled(bool enabled) {
    Renderer::setEnabled(enabled);

    if (enabled)
        viewer()->m_perspective = true;
}

void CrystalRenderer::display() {
    const auto currentState = State::currentState();

    static bool wireframe = true;
    static float tileScale = 1.0f;
    static float tileSpacing = 0.87f;
    static int count = 9;

    if (ImGui::BeginMenu("Crystal")) {
        ImGui::Checkbox("Wireframe", &wireframe);
        ImGui::SliderFloat("Tile scale", &tileScale, 0.f, 4.f);
        ImGui::SliderFloat("Tile spacing", &tileSpacing, 0.f, 4.f);
        ImGui::SliderInt("Hex Count", &count, 1, 100);

        ImGui::EndMenu();
    }

    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);

    const auto shader = shaderProgram("crystal");
    if (!shader)
        return;

    const auto num_cols = static_cast<int>(std::ceil(std::sqrt(count)));
    const auto num_rows = (count - 1) / num_cols + 1;
    const float scale = tileScale / static_cast<float>(num_cols);
    const float horizontal_space = scale * 1.5f;
    const float vertical_space = std::sqrt(3.f) * scale;
    /* This looks a bit scary, but it's just doing whatever this line:
     * gl_Position = vec4((col - 0.5 * (num_cols-1)) * horizontal_space, vertical_space * 0.5 * (row - num_rows + (num_cols != 1 ? 1.5 : 1.0)), 0.0, 1.0);
     * was doing in the shader, which just moves the hex grid to be centered in the bounding box cube.
     * This makes the shader part much simpler to read.
     */
    const mat4 mTrans = translate(
                                mat4{1.f},
                                vec3{
                                        -0.5f * static_cast<float>(num_cols - 1),
                                        (num_cols != 1 ? 1.5f : 1.f) - static_cast<float>(num_rows),
                                        0.f
                                }
                        );
    const mat4 mScale = glm::scale(
            mat4{1.f},
            vec3{horizontal_space, vertical_space * 0.5f, 1.f}
    );
    const mat4 model = mScale * mTrans;
    viewer()->setModelTransform(mat4{1.f}); // Setting the model matrix to identity because something keeps fucking it up
    const mat4 modelViewProjectionMatrix = viewer()->modelViewProjectionTransform();

    shader->use();
    shader->setUniform("MVP", modelViewProjectionMatrix);
    shader->setUniform("POINT_COUNT", count);
    shader->setUniform("num_cols", num_cols);
    shader->setUniform("num_rows", num_rows);
    shader->setUniform("tile_scale", scale);
    shader->setUniform("horizontal_space", horizontal_space);
    shader->setUniform("vertical_space", vertical_space);
    shader->setUniform("disp_mat", model);

    m_vao->drawArraysInstanced(GL_POINTS, 0, 1, count);
    m_vao->unbind();

    globjects::Program::release();

    currentState->apply();
}