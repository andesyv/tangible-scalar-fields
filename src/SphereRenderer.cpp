#include "SphereRenderer.h"
#include <globjects/base/File.h>
#include <iostream>
#include <imgui.h>
#include "Viewer.h"
#include "Scene.h"
#include "Protein.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include "SSAO.h"

using namespace molumes;
using namespace gl;
using namespace glm;
using namespace globjects;

SphereRenderer::SphereRenderer(Viewer* viewer) : Renderer(viewer)
{
	m_vertices->setData(viewer->scene()->protein()->atoms(), GL_STATIC_DRAW);
	m_atomData->setData(std::array<float, 8>({ 1.0f,0.0f,0.0f,1.0f,  1.0f,1.0f,1.0f,2.0f }), GL_STATIC_DRAW);
		
	m_size = static_cast<GLsizei>(viewer->scene()->protein()->atoms().size());

	auto vertexBinding = m_vao->binding(0);
	vertexBinding->setAttribute(0);
	vertexBinding->setBuffer(m_vertices.get(), 0, sizeof(vec4));
	vertexBinding->setFormat(4, GL_FLOAT);
	m_vao->enable(0);
	m_vao->unbind();

	m_intersectionBuffer->setData(sizeof(vec3) * 1024*1024*128 + sizeof(uint), nullptr, GL_DYNAMIC_DRAW);

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

	m_vertexShaderSourceImage = Shader::sourceFromFile("./res/sphere/image-vs.glsl");
	m_geometryShaderSourceImage = Shader::sourceFromFile("./res/sphere/image-gs.glsl");
	m_fragmentShaderSourceSpawn = Shader::sourceFromFile("./res/sphere/spawn-fs.glsl");
	m_fragmentShaderSourceShade = Shader::sourceFromFile("./res/sphere/shade-fs.glsl");

	m_vertexShaderImage = Shader::create(GL_VERTEX_SHADER, m_vertexShaderSourceImage.get());
	m_geometryShaderImage = Shader::create(GL_GEOMETRY_SHADER, m_geometryShaderSourceImage.get());
	m_fragmentShaderSpawn = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderSourceSpawn.get());
	m_fragmentShaderShade = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderSourceShade.get());

	m_programSpawn->attach(m_vertexShaderSphere.get(), m_geometryShaderSphere.get(), m_fragmentShaderSpawn.get());
	m_programShade->attach(m_vertexShaderImage.get(), m_geometryShaderImage.get(), m_fragmentShaderShade.get());

	m_framebufferSize = viewer->viewportSize();

	for (size_t i = 0; i < m_frameBuffers.size(); ++i)
	{
		m_positionTextures[i] = Texture::create(GL_TEXTURE_2D);
		m_positionTextures[i]->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		m_positionTextures[i]->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		m_positionTextures[i]->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		m_positionTextures[i]->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		m_positionTextures[i]->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

		m_normalTextures[i] = Texture::create(GL_TEXTURE_2D);
		m_normalTextures[i]->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		m_normalTextures[i]->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		m_normalTextures[i]->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		m_normalTextures[i]->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		m_normalTextures[i]->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

		m_depthTextures[i] = Texture::create(GL_TEXTURE_2D);
		m_depthTextures[i]->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		m_depthTextures[i]->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		m_depthTextures[i]->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		m_depthTextures[i]->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		m_depthTextures[i]->image2D(0, GL_DEPTH_COMPONENT, m_framebufferSize, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);

		m_frameBuffers[i] = Framebuffer::create();
		m_frameBuffers[i]->attachTexture(GL_COLOR_ATTACHMENT0, m_positionTextures[i].get());
		m_frameBuffers[i]->attachTexture(GL_COLOR_ATTACHMENT1, m_normalTextures[i].get());
		m_frameBuffers[i]->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTextures[i].get());
		m_frameBuffers[i]->setDrawBuffers({ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 });

		m_frameBuffers[i]->printStatus();
	}

	m_offsetTexture = Texture::create(GL_TEXTURE_2D);
	m_offsetTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_offsetTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_offsetTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_offsetTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_offsetTexture->image2D(0, GL_R32UI, m_framebufferSize, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);

	m_ssao = std::make_unique<SSAO>();
}

std::list<globjects::File*> SphereRenderer::shaderFiles() const
{
	return std::list<globjects::File*>({ 
		m_vertexShaderSourceSphere.get(),
		m_geometryShaderSourceSphere.get(),
		m_fragmentShaderSourceSphere.get(),
		m_vertexShaderSourceImage.get(),
		m_geometryShaderSourceImage.get(),
		m_fragmentShaderSourceSpawn.get(),
		m_fragmentShaderSourceShade.get()
	});
}

void SphereRenderer::display()
{
	if (viewer()->viewportSize() != m_framebufferSize)
	{
		m_framebufferSize = viewer()->viewportSize();
		m_offsetTexture->image2D(0, GL_R32UI, m_framebufferSize, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);

		for (size_t i = 0; i < m_frameBuffers.size(); ++i)
		{
			m_positionTextures[i]->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			m_normalTextures[i]->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			m_depthTextures[i]->image2D(0, GL_DEPTH_COMPONENT, m_framebufferSize, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);
		}
		
	}
	
	mat4 modelViewProjection = viewer()->modelViewProjectionTransform();
	mat4 inverseModelViewProjection = inverse(modelViewProjection);
	float sphereRadius = 1.0;
	float probeRadius = 1.5;
	float extendedSphereRadius = sphereRadius + probeRadius;

	m_frameBuffers[0]->bind();
	glClearDepth(1.0f);
	glClearColor(0.0, 0.0, 0.0, 65535.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	m_programSphere->setUniform("projection", viewer()->projectionTransform());
	m_programSphere->setUniform("modelView", viewer()->modelViewTransform());
	m_programSphere->setUniform("modelViewProjection", modelViewProjection);
	m_programSphere->setUniform("inverseModelViewProjection", inverseModelViewProjection);
	m_programSpawn->setUniform("radiusOffset", 0.0f);

	m_programSphere->use();

	m_vao->bind();
	m_vao->drawArrays(GL_POINTS, 0, m_size);
	m_vao->unbind();

	m_programSphere->release();

	//m_ssao->display(mat4(1.0f)/*viewer()->modelViewTransform()*/,viewer()->projectionTransform(), m_frameBuffer->id(), m_depthTexture->id(), m_normalTexture->id());


	m_positionTextures[0]->bindActive(0);
	m_normalTextures[0]->bindActive(1);
	m_depthTextures[0]->bindActive(2);
	
	const uint intersectionClearValue = 1;
	m_intersectionBuffer->clearSubData(GL_R32UI, 0, sizeof(uint), GL_RED_INTEGER, GL_UNSIGNED_INT, &intersectionClearValue);

	const uint offsetClearValue = 0;
	m_offsetTexture->clearImage(0, GL_RED_INTEGER, GL_UNSIGNED_INT, &offsetClearValue);

	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	glDepthFunc(GL_ALWAYS);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_FALSE);

	m_positionTextures[0]->bindActive(0);
	m_offsetTexture->bindImageTexture(0, 0, false, 0, GL_READ_WRITE, GL_R32UI);
	m_atomData->bindBase(GL_UNIFORM_BUFFER, 0);
	
	m_programSpawn->setUniform("projection", viewer()->projectionTransform());
	m_programSpawn->setUniform("modelView", viewer()->modelViewTransform());
	m_programSpawn->setUniform("modelViewProjection", modelViewProjection);
	m_programSpawn->setUniform("inverseModelViewProjection", inverseModelViewProjection);
	m_programSpawn->setUniform("radiusOffset", 1.4f);
	m_programSpawn->use();

	m_vao->bind();
	m_vao->drawArrays(GL_POINTS, 0, m_size);
	m_vao->unbind();

	m_programSpawn->release();
	m_frameBuffers[0]->unbind();

	m_positionTextures[0]->unbindActive(0);
	m_intersectionBuffer->unbind(GL_SHADER_STORAGE_BUFFER);
	m_offsetTexture->unbindImageTexture(0);

	glMemoryBarrier(GL_ALL_BARRIER_BITS);// GL_TEXTURE_FETCH_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);


	static vec3 ambientMaterial(0.04f, 0.04f, 0.04f);
	static vec3 diffuseMaterial(0.75, 0.75f, 0.75f);
	static vec3 specularMaterial(0.5f, 0.5f, 0.5f);
	static float shininess = 16.0f;
	static float softness = 1.0;

	ImGui::Begin("Sphere Renderer");
	ImGui::ColorEdit3("Ambient", (float*)&ambientMaterial);
	ImGui::ColorEdit3("Diffuse", (float*)&diffuseMaterial);
	ImGui::ColorEdit3("Specular", (float*)&specularMaterial);
	ImGui::SliderFloat("Shininess", &shininess, 1.0f, 256.0f);
	ImGui::SliderFloat("Softness", &softness, 1.0f, 16.0f);
	ImGui::End();


//	uint sphereCount = 0;
//	m_verticesSpawn->getSubData(0, sizeof(uint), &sphereCount);
	//std::array<vec3,1> sphereValues = m_verticesSpawn->getSubData<vec3,1>(16);
	//std::cout << "SphereCount: " << sphereCount << std::endl;
	//std::cout << "SphereValue: " << to_string(sphereValues[0]) << std::endl;

	//std::cout << to_string(inverse(viewer()->projectionTransform())*vec4(0.5, 0.5, 0.0, 1.0)) << std::endl;

	m_frameBuffers[1]->bind();
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);

	glClearDepth(1.0f);
	glClearColor(0.0, 0.0, 0.0, 65535.0f);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	//Framebuffer::defaultFBO()->bind();

	m_positionTextures[0]->bindActive(0);
	m_normalTextures[0]->bindActive(1);
	m_depthTextures[0]->bindActive(2);
	m_offsetTexture->bindActive(3);
	m_intersectionBuffer->bindBase(GL_SHADER_STORAGE_BUFFER, 1);

	vec3 lightPosition = normalize(vec3(inverse(viewer()->modelViewTransform())*vec4(0.0f, 0.0f, -1.0f,0.0)));

	m_programShade->setUniform("modelView", viewer()->modelViewTransform());
	m_programShade->setUniform("projection", viewer()->projectionTransform());
	m_programShade->setUniform("inverseProjection", inverse(viewer()->projectionTransform()));
	m_programShade->setUniform("modelViewProjection", viewer()->modelViewProjectionTransform());
	m_programShade->setUniform("inverseModelViewProjection", inverseModelViewProjection);
	m_programShade->setUniform("probeRadius", 1.0f);
	m_programShade->setUniform("lightPosition", lightPosition);
	m_programShade->setUniform("ambientMaterial", ambientMaterial);
	m_programShade->setUniform("diffuseMaterial", diffuseMaterial);
	m_programShade->setUniform("specularMaterial", specularMaterial);
	m_programShade->setUniform("shininess", shininess);
	m_programShade->setUniform("colorTexture", 0);
	m_programShade->setUniform("normalTexture", 1);
	m_programShade->setUniform("depthTexture", 2);
	m_programShade->setUniform("offsetTexture", 3);
	m_programShade->setUniform("softness", softness);
	m_programShade->use();

	m_vaoQuad->bind();
	m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
	m_vaoQuad->unbind();

	m_programShade->release();

	m_intersectionBuffer->unbind(GL_SHADER_STORAGE_BUFFER);
	m_offsetTexture->unbindActive(3);
	m_depthTextures[1]->unbindActive(2);
	m_normalTextures[1]->unbindActive(1);
	m_positionTextures[1]->unbindActive(0);
	m_atomData->unbind(GL_UNIFORM_BUFFER);

	m_ssao->display(viewer()->modelViewTransform(), viewer()->projectionTransform(), m_frameBuffers[1]->id(), m_depthTextures[1]->id(), m_normalTextures[1]->id());

	m_frameBuffers[1]->blit(GL_COLOR_ATTACHMENT0, {0,0,viewer()->viewportSize().x, viewer()->viewportSize().y}, Framebuffer::defaultFBO().get(), GL_BACK_LEFT, { 0,0,viewer()->viewportSize().x, viewer()->viewportSize().y }, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	m_frameBuffers[1]->unbind();
}