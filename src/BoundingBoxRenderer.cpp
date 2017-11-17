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

	
	static std::array< std::array<GLushort, 4>, 6> indices{ {
		// front
		{0,1,2,3},
		// top
		{1,5,6,2},
		// back
		{7,6,5,4},
		// bottom
		{4,0,3,7},
		// left
		{4,5,1,0},
		// right
		{3,2,6,7}
	}};

	m_indices->setData(indices, GL_STATIC_DRAW);
	m_vertices->setData(vertices, GL_STATIC_DRAW);

	m_size = static_cast<GLsizei>(indices.size()*indices.front().size());
	m_vao->bindElementBuffer(m_indices.get());

	auto vertexBinding = m_vao->binding(0);
	vertexBinding->setAttribute(0);
	vertexBinding->setBuffer(m_vertices.get(), 0, sizeof(vec3));
	vertexBinding->setFormat(3, GL_FLOAT, GL_TRUE);
	m_vao->enable(0);

	m_vao->unbind();

	m_vertexShaderSource = Shader::sourceFromFile("./res/boundingbox/boundingbox-vs.glsl");
	m_tesselationControlShaderSource = Shader::sourceFromFile("./res/boundingbox/boundingbox-tcs.glsl");
	m_tesselationEvaluationShaderSource = Shader::sourceFromFile("./res/boundingbox/boundingbox-tes.glsl");
	m_geometryShaderSource = Shader::sourceFromFile("./res/boundingbox/boundingbox-gs.glsl");
	m_fragmentShaderSource = Shader::sourceFromFile("./res/boundingbox/boundingbox-fs.glsl");

	m_vertexShader = Shader::create(GL_VERTEX_SHADER, m_vertexShaderSource.get());
	m_tesselationControlShader = Shader::create(GL_TESS_CONTROL_SHADER, m_tesselationControlShaderSource.get());
	m_tesselationEvaluationShader = Shader::create(GL_TESS_EVALUATION_SHADER, m_tesselationEvaluationShaderSource.get());
	m_geometryShader = Shader::create(GL_GEOMETRY_SHADER, m_geometryShaderSource.get());
	m_fragmentShader = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderSource.get());

	m_program->attach(m_vertexShader.get(), m_tesselationControlShader.get(), m_tesselationEvaluationShader.get(), m_geometryShader.get(), m_fragmentShader.get());
}

std::list<globjects::File*> BoundingBoxRenderer::shaderFiles() const
{
	return std::list<globjects::File*>({ 
		m_vertexShaderSource.get(),
		m_tesselationControlShaderSource.get(),
		m_tesselationEvaluationShaderSource.get(),
		m_geometryShaderSource.get(),
		m_fragmentShaderSource.get()
	});
}

void BoundingBoxRenderer::display()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_program->setUniform("Projection", viewer()->projectionTransform());
	m_program->setUniform("Modelview", viewer()->modelViewTransform());
	m_program->setUniform("NormalMatrix", inverse(mat3(viewer()->modelViewTransform())) );

	m_program->setUniform("TessLevelInner", 1.0f);
	m_program->setUniform("TessLevelOuter", 1.0f);

	m_program->setUniform("LightPosition", vec3(0.25f, 0.25f, 0.75f));
	m_program->setUniform("AmbientMaterial", vec3(0.04f, 0.04f, 0.04f));
	m_program->setUniform("DiffuseMaterial", vec3(0.0f, 0.75f, 0.75f));
	m_program->setUniform("SpecularMaterial", vec3(0.5f, 0.5f, 0.5f));
	m_program->setUniform("Shininess", 50.0f);
	m_program->use();

	m_vao->bind();
	glPatchParameteri(GL_PATCH_VERTICES, 4);
	m_vao->drawElements(GL_PATCHES, m_size, GL_UNSIGNED_SHORT, nullptr);
	m_vao->unbind();

	glDisable(GL_BLEND);
	glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
}