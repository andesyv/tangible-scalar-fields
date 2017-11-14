#include "BoundingBoxRenderer.h"
#include <globjects/base/File.h>
#include <iostream>
#include "Viewer.h"

using namespace molumes;
using namespace gl;
using namespace glm;
using namespace globjects;

BoundingBoxRenderer::BoundingBoxRenderer(Viewer* viewer) : Renderer(viewer)
{
	static std::array<vec3, 8> vertices {{
		// front
		{-1.0, -1.0, 1.0 },
		{ 1.0, -1.0, 1.0 },
		{ 1.0, 1.0, 1.0 },
		{ -1.0, 1.0, 1.0 },
		// back
		{ -1.0, -1.0, -1.0 },
		{ 1.0, -1.0, -1.0 },
		{ 1.0, 1.0, -1.0 },
		{ -1.0, 1.0, -1.0 }
	}};

	static std::array< std::array<GLushort, 2>, 24> indices{ {
		// front
		{0,1}, {1,2}, {2,3}, {3,0},
		// top
		{1,5}, {5,6}, {6,2}, {2,1}, 
		// back
		{7,6}, {6,5}, {5,4}, {4,7},
		// bottom
		{4,0}, {0,3}, {3,7}, {7,4},
		// left
		{4,5}, {5,1}, {1,0}, {0,4},
		// right
		{3,2}, {2,6}, {6,7}, {7,3}
	} };

	m_indices->setData(indices, GL_STATIC_DRAW);
	m_vertices->setData(vertices, GL_STATIC_DRAW);

	m_size = static_cast<GLsizei>(indices.size() * 3);

	m_vao->bindElementBuffer(m_indices.get());

	auto vertexBinding = m_vao->binding(0);
	vertexBinding->setAttribute(0);
	vertexBinding->setBuffer(m_vertices.get(), 0, sizeof(vec3));
	vertexBinding->setFormat(3, GL_FLOAT, GL_TRUE);
	m_vao->enable(0);

	m_vao->unbind();

	m_vertexShaderSource = Shader::sourceFromFile("./res/boundingbox.vert");
	m_fragmentShaderSource = Shader::sourceFromFile("./res/boundingbox.frag");

	m_vertexShader = Shader::create(GL_VERTEX_SHADER,m_vertexShaderSource.get());
	m_fragmentShader = Shader::create(GL_FRAGMENT_SHADER,m_fragmentShaderSource.get());

	m_program->attach(m_vertexShader.get(), m_fragmentShader.get());
}

void BoundingBoxRenderer::display()
{
	m_program->use();
	m_program->setUniform("u_modelViewProjection", viewer()->modelViewProjectionTransform());

	m_vao->bind();
	m_vao->drawElements(GL_LINES, m_size, GL_UNSIGNED_SHORT, nullptr);
	m_vao->unbind();

	m_program->release();
}