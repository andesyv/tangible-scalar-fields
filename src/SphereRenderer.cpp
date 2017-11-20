#include "SphereRenderer.h"
#include <globjects/base/File.h>
#include <iostream>
#include "Viewer.h"
#include "Scene.h"
#include "Protein.h"

using namespace molumes;
using namespace gl;
using namespace glm;
using namespace globjects;

SphereRenderer::SphereRenderer(Viewer* viewer) : Renderer(viewer)
{
	m_vertices->setData(viewer->scene()->protein()->atoms(), GL_STATIC_DRAW);
	m_size = static_cast<GLsizei>(viewer->scene()->protein()->atoms().size());
//	m_vao->bindElementBuffer(m_vertices.get());

	auto vertexBinding = m_vao->binding(0);
	vertexBinding->setAttribute(0);
	vertexBinding->setBuffer(m_vertices.get(), 0, sizeof(vec3));
	vertexBinding->setFormat(3, GL_FLOAT);
	m_vao->enable(0);
	m_vao->unbind();
	
	m_vertexShaderSource = Shader::sourceFromFile("./res/sphere/sphere-vs.glsl");
	m_geometryShaderSource = Shader::sourceFromFile("./res/sphere/sphere-gs.glsl");
	m_fragmentShaderSource = Shader::sourceFromFile("./res/sphere/sphere-fs.glsl");

	m_vertexShader = Shader::create(GL_VERTEX_SHADER, m_vertexShaderSource.get());
	m_geometryShader = Shader::create(GL_GEOMETRY_SHADER, m_geometryShaderSource.get());
	m_fragmentShader = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderSource.get());

	m_program->attach(m_vertexShader.get(), m_geometryShader.get(), m_fragmentShader.get());
}

std::list<globjects::File*> SphereRenderer::shaderFiles() const
{
	return std::list<globjects::File*>({ 
		m_vertexShaderSource.get(),
		m_geometryShaderSource.get(),
		m_fragmentShaderSource.get()
	});
}

void SphereRenderer::display()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	m_program->setUniform("projection", viewer()->projectionTransform());
	m_program->setUniform("modelView", viewer()->modelViewTransform());
	m_program->setUniform("sphereRadius", 1.0f);
	
	m_program->setUniform("lightPosition", normalize(vec3(-1.0f, 1.0f, 0.0f)));
	m_program->setUniform("ambientMaterial", vec3(0.04f, 0.04f, 0.04f));
	m_program->setUniform("diffuseMaterial", vec3(0.75, 0.75f, 0.75f));
	m_program->setUniform("specularMaterial", vec3(0.5f, 0.5f, 0.5f));
	m_program->setUniform("shininess", 60.0f);
	m_program->use();

	m_vao->bind();
	m_vao->drawArrays(GL_POINTS, 0, m_size);
	m_vao->unbind();

	m_program->release();
}