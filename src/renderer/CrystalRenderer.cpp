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

constexpr auto CUBEVERTICES = std::to_array({
		-1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f,-1.0f,
		1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f,-1.0f,
		1.0f,-1.0f,-1.0f,
		1.0f, 1.0f,-1.0f,
		1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f,-1.0f,
		1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f,-1.0f, 1.0f,
		1.0f,-1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f,-1.0f,-1.0f,
		1.0f, 1.0f,-1.0f,
		1.0f,-1.0f,-1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f,-1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f,-1.0f,
		-1.0f, 1.0f,-1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f,-1.0f, 1.0f});

CrystalRenderer::CrystalRenderer(Viewer* viewer) : Renderer(viewer)
{
	m_vao = std::make_unique<VertexArray>(); /// Apparantly exactly the same as VertexArray::create();
	m_vertexBuffer = Buffer::create();
	m_vertexBuffer->setData(CUBEVERTICES, GL_STATIC_DRAW);

	const auto binding = m_vao->binding(0);
	binding->setAttribute(0);
	binding->setBuffer(m_vertexBuffer.get(), 0, sizeof(vec3));
	binding->setFormat(3, GL_FLOAT);
	m_vao->enable(0);

	createShaderProgram("crystal", {
		{ GL_VERTEX_SHADER, "./res/crystal/crystal-vs.glsl" },
		{ GL_FRAGMENT_SHADER, "./res/crystal/crystal-fs.glsl" }
		});
}

void CrystalRenderer::display()
{
	const auto currentState = State::currentState();

	const auto shader = shaderProgram("crystal");
	if (!shader)
		return;
	
	viewer()->setModelTransform(mat4{1.f}); // Setting the model matrix to identity because something keeps fucking it up
	const mat4 modelViewProjectionMatrix = viewer()->modelViewProjectionTransform();

	shader->use();
	shader->setUniform("MVP", modelViewProjectionMatrix);
	
	m_vao->bind();
	m_vao->drawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(CUBEVERTICES.size()));
	m_vao->unbind();

	shader->release();

	currentState->apply();
}