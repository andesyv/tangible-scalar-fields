#include "SphereRenderer.h"
#include <globjects/base/File.h>
#include <iostream>
#include <imgui.h>
#include "Viewer.h"
#include "Scene.h"
#include "Protein.h"

//#include "SSAO.h"

using namespace molumes;
using namespace gl;
using namespace glm;
using namespace globjects;

SphereRenderer::SphereRenderer(Viewer* viewer) : Renderer(viewer)
{
	m_vertices->setData(viewer->scene()->protein()->atoms(), GL_STATIC_DRAW);
	m_size = static_cast<GLsizei>(viewer->scene()->protein()->atoms().size());

	auto vertexBinding = m_vao->binding(0);
	vertexBinding->setAttribute(0);
	vertexBinding->setBuffer(m_vertices.get(), 0, sizeof(vec3));
	vertexBinding->setFormat(3, GL_FLOAT);
	m_vao->enable(0);
	m_vao->unbind();
	
	m_verticesQuad->setData(std::array<vec3, 1>({ vec3(0.0f, 0.0f, 0.0f) }), GL_STATIC_DRAW);
	auto vertexBindingQuad = m_vaoQuad->binding(0);
	vertexBindingQuad->setBuffer(m_verticesQuad.get(), 0, sizeof(vec3));
	vertexBindingQuad->setFormat(3, GL_FLOAT);
	m_vaoQuad->enable(0);
	m_vaoQuad->unbind();

	m_vertexShaderSourceSphere = Shader::sourceFromFile("./res/sphere/sphere-vs.glsl");
	m_geometryShaderSourceSphere = Shader::sourceFromFile("./res/sphere/sphere-gs.glsl");
	m_fragmentShaderSourceSphere = Shader::sourceFromFile("./res/sphere/sphere-fs.glsl");

	m_vertexShaderSphere = Shader::create(GL_VERTEX_SHADER, m_vertexShaderSourceSphere.get());
	m_geometryShaderSphere = Shader::create(GL_GEOMETRY_SHADER, m_geometryShaderSourceSphere.get());
	m_fragmentShaderSphere = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderSourceSphere.get());

	m_programSphere->attach(m_vertexShaderSphere.get(), m_geometryShaderSphere.get(), m_fragmentShaderSphere.get());

	m_vertexShaderSourceShade = Shader::sourceFromFile("./res/sphere/shade-vs.glsl");
	m_geometryShaderSourceShade = Shader::sourceFromFile("./res/sphere/shade-gs.glsl");
	m_fragmentShaderSourceShade = Shader::sourceFromFile("./res/sphere/shade-fs.glsl");

	m_vertexShaderShade = Shader::create(GL_VERTEX_SHADER, m_vertexShaderSourceShade.get());
	m_geometryShaderShade = Shader::create(GL_GEOMETRY_SHADER, m_geometryShaderSourceShade.get());
	m_fragmentShaderShade = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderSourceShade.get());

	m_programShade->attach(m_vertexShaderShade.get(), m_geometryShaderShade.get(), m_fragmentShaderShade.get());
	m_framebufferSize = viewer->viewportSize();

	m_colorTexture =  Texture::create(GL_TEXTURE_2D);
	m_colorTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_colorTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_colorTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_colorTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_colorTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_normalTexture = Texture::create(GL_TEXTURE_2D);
	m_normalTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_normalTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_normalTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_normalTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_normalTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_depthTexture = Texture::create(GL_TEXTURE_2D);
	m_depthTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_depthTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_depthTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_depthTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_depthTexture->image2D(0, GL_DEPTH_COMPONENT32F, m_framebufferSize, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);

	m_frameBuffer = Framebuffer::create();
	m_frameBuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_colorTexture.get());
	m_frameBuffer->attachTexture(GL_COLOR_ATTACHMENT1, m_normalTexture.get());
	m_frameBuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
	m_frameBuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 });

	m_frameBuffer->printStatus();

	//m_ssao = std::make_unique<SSAO>();
}

std::list<globjects::File*> SphereRenderer::shaderFiles() const
{
	return std::list<globjects::File*>({ 
		m_vertexShaderSourceSphere.get(),
		m_geometryShaderSourceSphere.get(),
		m_fragmentShaderSourceSphere.get(),
		m_vertexShaderSourceShade.get(),
		m_geometryShaderSourceShade.get(),
		m_fragmentShaderSourceShade.get()
	});
}

void SphereRenderer::display()
{
	if (viewer()->viewportSize() != m_framebufferSize)
	{
		m_framebufferSize = viewer()->viewportSize();
		m_colorTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		m_normalTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		m_depthTexture->image2D(0, GL_DEPTH_COMPONENT32F, m_framebufferSize, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);
		
	}
	
	m_frameBuffer->bind();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	m_programSphere->setUniform("projection", viewer()->projectionTransform());
	m_programSphere->setUniform("modelView", viewer()->modelViewTransform());
	m_programSphere->setUniform("sphereRadius", 1.0f);
	

	m_programSphere->setUniform("normalMatrix", mat3(1.0f));//mat3(transpose(viewer()->modelViewTransform())));
	m_programSphere->use();

	m_vao->bind();
	m_vao->drawArrays(GL_POINTS, 0, m_size);
	m_vao->unbind();

	m_programSphere->release();
	m_frameBuffer->unbind();

	//m_ssao->display(mat4(1.0f)/*viewer()->modelViewTransform()*/,viewer()->projectionTransform(), m_frameBuffer->id(), m_depthTexture->id(), m_normalTexture->id());

	Framebuffer::defaultFBO()->bind();

	m_colorTexture->bindActive(0);
	m_normalTexture->bindActive(1);
	m_depthTexture->bindActive(2);


	static vec3 ambientMaterial(0.04f, 0.04f, 0.04f);
	static vec3 diffuseMaterial(0.75, 0.75f, 0.75f);
	static vec3 specularMaterial(0.5f, 0.5f, 0.5f);
	static float shininess = 16.0f;

	ImGui::Begin("Sphere Renderer");
	ImGui::ColorEdit3("Ambient", (float*)&ambientMaterial);
	ImGui::ColorEdit3("Diffuse", (float*)&diffuseMaterial);
	ImGui::ColorEdit3("Specular", (float*)&specularMaterial);
	ImGui::SliderFloat("Shininess", &shininess, 1.0f, 256.0f);
	ImGui::End();

	m_programShade->setUniform("lightPosition", normalize(vec3(0.0f, 0.0f, -1.0f)));
	m_programShade->setUniform("ambientMaterial", ambientMaterial);
	m_programShade->setUniform("diffuseMaterial", diffuseMaterial);
	m_programShade->setUniform("specularMaterial", specularMaterial);
	m_programShade->setUniform("shininess", shininess);
	m_programShade->setUniform("colorTexture", 0);
	m_programShade->setUniform("normalTexture", 1);
	m_programShade->setUniform("depthTexture", 2);
	m_programShade->use();

	m_vaoQuad->bind();
	m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
	m_vaoQuad->unbind();
	m_programSphere->release();

	m_depthTexture->unbindActive(2);
	m_normalTexture->unbindActive(1);
	m_colorTexture->unbindActive(0);

	//m_frameBuffer->blit(GL_COLOR_ATTACHMENT0, {0,0,viewer()->viewportSize().x, viewer()->viewportSize().y}, Framebuffer::defaultFBO().get(), GL_BACK_LEFT, { 0,0,viewer()->viewportSize().x, viewer()->viewportSize().y }, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}