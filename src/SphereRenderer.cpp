#include "SphereRenderer.h"
#include <globjects/base/File.h>
#include <iostream>
#include <imgui.h>
#include "Viewer.h"
#include "Scene.h"
#include "Protein.h"
#include "SSAO.h"
#include <lodepng.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

using namespace molumes;
using namespace gl;
using namespace glm;
using namespace globjects;

struct Statistics
{
	uint intersectionCount = 0;
	uint totalPixelCount = 0;
	uint totalEntryCount = 0;
	uint maximumEntryCount = 0;
};

SphereRenderer::SphereRenderer(Viewer* viewer) : Renderer(viewer)
{
	Statistics s;

	//m_vertices->setData(viewer->scene()->protein()->atoms(), GL_STATIC_DRAW);
	m_vertices->setStorage(viewer->scene()->protein()->atoms(),gl::GL_NONE_BIT);
	m_elementColorsRadii->setStorage(viewer->scene()->protein()->activeElementColorsRadiiPacked(), gl::GL_NONE_BIT);
	m_residueColors->setStorage(viewer->scene()->protein()->activeResidueColorsPacked(), gl::GL_NONE_BIT);
	m_chainColors->setStorage(viewer->scene()->protein()->activeChainColorsPacked(), gl::GL_NONE_BIT);
	
	m_size = static_cast<GLsizei>(viewer->scene()->protein()->atoms().size());

	auto vertexBinding = m_vao->binding(0);
	vertexBinding->setAttribute(0);
	vertexBinding->setBuffer(m_vertices.get(), 0, sizeof(vec4));
	vertexBinding->setFormat(4, GL_FLOAT);
	m_vao->enable(0);
	m_vao->unbind();

	m_intersectionBuffer->setStorage(sizeof(vec3) * 1024*1024*128 + sizeof(uint), nullptr, gl::GL_NONE_BIT);
	m_statisticsBuffer->setStorage(sizeof(s), (void*)&s, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);

	m_verticesQuad->setStorage(std::array<vec3, 1>({ vec3(0.0f, 0.0f, 0.0f) }), gl::GL_NONE_BIT);
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
	m_fragmentShaderSourceBlend = Shader::sourceFromFile("./res/sphere/blend-fs.glsl");

	m_vertexShaderImage = Shader::create(GL_VERTEX_SHADER, m_vertexShaderSourceImage.get());
	m_geometryShaderImage = Shader::create(GL_GEOMETRY_SHADER, m_geometryShaderSourceImage.get());
	m_fragmentShaderSpawn = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderSourceSpawn.get());
	m_fragmentShaderShade = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderSourceShade.get());
	m_fragmentShaderBlend = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderSourceBlend.get());

	m_programSpawn->attach(m_vertexShaderSphere.get(), m_geometryShaderSphere.get(), m_fragmentShaderSpawn.get());
	m_programShade->attach(m_vertexShaderImage.get(), m_geometryShaderImage.get(), m_fragmentShaderShade.get());
	m_programBlend->attach(m_vertexShaderImage.get(), m_geometryShaderImage.get(), m_fragmentShaderBlend.get());

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

	m_environmentTexture = Texture::create(GL_TEXTURE_2D);
	m_environmentTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_environmentTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_environmentTexture->setParameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
	m_environmentTexture->setParameter(GL_TEXTURE_WRAP_T, GL_REPEAT);

	std::string environmentFilename = "./dat/Env_Lat-Lon.png";
	uint environmentWidth, environmentHeight;
	std::vector<unsigned char> environmentImage;
	uint error = lodepng::decode(environmentImage, environmentWidth, environmentHeight, environmentFilename);

	if (error)
		globjects::debug() << "Could not load " << environmentFilename << "!";
	else
	{
		m_environmentTexture->image2D(0, GL_RGBA, environmentWidth, environmentHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)&environmentImage.front());
		//glGenerateMipmap(GL_TEXTURE_2D);
	}


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
		m_fragmentShaderSourceShade.get(),
		m_fragmentShaderSourceBlend.get()
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
	
/*	static vec3 ambientMaterial(0.04f, 0.04f, 0.04f);
	static vec3 diffuseMaterial(0.75, 0.75f, 0.75f);
	static vec3 specularMaterial(0.5f, 0.5f, 0.5f);
*/
	// nice color scheme
	static vec3 ambientMaterial(0.249f, 0.113f, 0.336f);
	static vec3 diffuseMaterial(0.042f, 0.683f, 0.572f);
	static vec3 specularMaterial(0.629f, 0.629f, 0.629f);
	static float shininess = 20.0f;

	static float sharpness = 1.5f;
	static bool ambientOcclusion = false;
	static bool environmentMapping = false;
	static int coloring = 0;

	ImGui::Begin("Sphere Renderer");
	ImGui::ColorEdit3("Ambient", (float*)&ambientMaterial);
	ImGui::ColorEdit3("Diffuse", (float*)&diffuseMaterial);
	ImGui::ColorEdit3("Specular", (float*)&specularMaterial);
	ImGui::SliderFloat("Shininess", &shininess, 1.0f, 256.0f);
	ImGui::SliderFloat("Sharpness", &sharpness, 0.75f, 16.0f);
	ImGui::Combo("Coloring", &coloring, "None\0Element\0Residue\0Chain\0");
	ImGui::Checkbox("Ambient Occlusion", &ambientOcclusion);
	ImGui::Checkbox("Environment Mapping", &environmentMapping);
	ImGui::End();

	mat4 view = viewer()->viewTransform();
	mat4 inverseView = inverse(view);
	mat4 modelView = viewer()->modelViewTransform();
	mat4 inverseModelView = inverse(modelView);
	mat4 modelViewProjection = viewer()->modelViewProjectionTransform();
	mat4 inverseModelViewProjection = inverse(modelViewProjection);

	const float contributingAtoms = 32.0f;
	float radiusScale = sqrtf(log(contributingAtoms*exp(sharpness)) / sharpness);

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
	m_programSphere->setUniform("radiusScale", 1.0f);

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
	m_elementColorsRadii->bindBase(GL_UNIFORM_BUFFER, 0);
	m_residueColors->bindBase(GL_UNIFORM_BUFFER, 1);
	m_chainColors->bindBase(GL_UNIFORM_BUFFER, 2);
	
	m_programSpawn->setUniform("projection", viewer()->projectionTransform());
	m_programSpawn->setUniform("modelView", viewer()->modelViewTransform());
	m_programSpawn->setUniform("modelViewProjection", modelViewProjection);
	m_programSpawn->setUniform("inverseModelViewProjection", inverseModelViewProjection);
	m_programSpawn->setUniform("radiusScale", radiusScale);
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
	glClearColor(0.0, 0.0, 0.0, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//Framebuffer::defaultFBO()->bind();

	m_positionTextures[0]->bindActive(0);
	m_normalTextures[0]->bindActive(1);
	m_depthTextures[0]->bindActive(2);
	m_environmentTexture->bindActive(3);
	m_offsetTexture->bindActive(4);
	m_intersectionBuffer->bindBase(GL_SHADER_STORAGE_BUFFER, 1);
	m_statisticsBuffer->bindBase(GL_SHADER_STORAGE_BUFFER, 2);

/*	double xpos, ypos;
	glfwGetCursorPos(viewer()->window(), &xpos, &ypos);
	xpos = 2.0 * xpos / double(viewer()->viewportSize().x) - 1.0;
	ypos = -(2.0 * ypos / double(viewer()->viewportSize().y) - 1.0);

	vec4 center = view * vec4(0.0f, 0.0f, 0.0f, 1.0);
	vec4 corner = view * vec4(1.0f, 1.0f, 1.0f, 1.0f);
	float radius = distance(corner, center);
	vec3 viewLightDirection = normalize(vec3(xpos, ypos, 1.0f));
	vec4 viewLightPosition = center + vec4(viewLightDirection, 0.0f) * radius;
	vec4 worldLightPosition = inverseModelView * viewLightPosition;

	std::cout << "CENTER:" << to_string(center) << std::endl;
	std::cout << "CORNER:" << to_string(corner) << std::endl;
	std::cout << "RADIUS:" << radius << std::endl;
	std::cout << "viewLightDirection:" << to_string(viewLightDirection) << std::endl;
	std::cout << "viewLightPosition:" << to_string(viewLightPosition) << std::endl;
	std::cout << "worldLightPosition:" << to_string(worldLightPosition) << std::endl << std::endl;
*/	
	//std::cout << xpos << "," << ypos << ":" << to_string(lightPosition) << std::endl << std::endl;

	m_programShade->setUniform("modelView", viewer()->modelViewTransform());
	m_programShade->setUniform("projection", viewer()->projectionTransform());
	m_programShade->setUniform("inverseProjection", inverse(viewer()->projectionTransform()));
	m_programShade->setUniform("modelViewProjection", viewer()->modelViewProjectionTransform());
	m_programShade->setUniform("inverseModelViewProjection", inverseModelViewProjection);
	m_programShade->setUniform("lightPosition", vec3(viewer()->worldLightPosition()));
	m_programShade->setUniform("ambientMaterial", ambientMaterial);
	m_programShade->setUniform("diffuseMaterial", diffuseMaterial);
	m_programShade->setUniform("specularMaterial", specularMaterial);
	m_programShade->setUniform("shininess", shininess);
	m_programShade->setUniform("colorTexture", 0);
	m_programShade->setUniform("normalTexture", 1);
	m_programShade->setUniform("depthTexture", 2);
	m_programShade->setUniform("environmentTexture", 3);
	m_programShade->setUniform("offsetTexture", 4);
	m_programShade->setUniform("sharpness", sharpness);
	m_programShade->setUniform("coloring", uint(coloring));
	m_programShade->setUniform("environment", environmentMapping);

	m_programShade->use();
	
	m_vaoQuad->bind();
	m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
	m_vaoQuad->unbind();

	m_programShade->release();

	m_intersectionBuffer->unbind(GL_SHADER_STORAGE_BUFFER);

	m_offsetTexture->unbindActive(4);
	m_environmentTexture->unbindActive(3);
	m_depthTextures[0]->unbindActive(2);
	m_normalTextures[0]->unbindActive(1);
	m_positionTextures[0]->unbindActive(0);

	m_chainColors->unbind(GL_UNIFORM_BUFFER);
	m_residueColors->unbind(GL_UNIFORM_BUFFER);
	m_elementColorsRadii->unbind(GL_UNIFORM_BUFFER);

	m_frameBuffers[1]->unbind();

	if (ambientOcclusion)
		m_ssao->display(viewer()->modelViewTransform(), viewer()->projectionTransform(), m_frameBuffers[1]->id(), m_depthTextures[1]->id(), m_normalTextures[1]->id());

	//m_frameBuffers[1]->blit(GL_COLOR_ATTACHMENT0, {0,0,viewer()->viewportSize().x, viewer()->viewportSize().y}, Framebuffer::defaultFBO().get(), GL_BACK, { 0,0,viewer()->viewportSize().x, viewer()->viewportSize().y }, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	Framebuffer::defaultFBO()->bind();
	glDepthFunc(GL_LEQUAL);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_positionTextures[1]->bindActive(0);
	m_normalTextures[1]->bindActive(1);
	m_depthTextures[1]->bindActive(2);
	m_environmentTexture->bindActive(3);

	m_programBlend->setUniform("inverseModelViewProjection", inverseModelViewProjection);
	m_programBlend->setUniform("colorTexture", 0);
	m_programBlend->setUniform("normalTexture", 1);
	m_programBlend->setUniform("depthTexture", 2);
	m_programBlend->setUniform("environmentTexture", 3);
	m_programBlend->setUniform("environment", environmentMapping);
	m_programBlend->use();

	m_vaoQuad->bind();
	m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
	m_vaoQuad->unbind();

	m_environmentTexture->unbindActive(3);
	m_depthTextures[1]->unbindActive(2);
	m_normalTextures[1]->unbindActive(1);
	m_positionTextures[1]->unbindActive(0);

	glDisable(GL_BLEND);
	m_programBlend->release();
	
#ifdef STATISTICS
	Statistics *s = (Statistics*) m_statisticsBuffer->map();	
	std::string intersectionCount = std::to_string(s->intersectionCount);
	std::stringstream ss;
	ss << std::fixed << std::setprecision(2) << (float(s->intersectionCount) * 28.0f) / (1024.0f * 1024.0f);
	std::string intersectionMemory = ss.str();
	std::string totalPixelCount = std::to_string(s->totalPixelCount);
	std::string totalEntryCount = std::to_string(s->totalEntryCount);
	std::string maximumEntryCount = std::to_string(s->maximumEntryCount);

	*s = Statistics();

	m_statisticsBuffer->unmap();

	ImGui::Begin("Statistics");

	ImGui::InputText("Intersection Count", (char*)intersectionCount.c_str(),intersectionCount.size(), ImGuiInputTextFlags_ReadOnly);
	ImGui::InputText("Intersection Memory (MB)", (char*)intersectionMemory.c_str(), intersectionMemory.size(), ImGuiInputTextFlags_ReadOnly);
	ImGui::InputText("Total Pixel Count", (char*)totalPixelCount.c_str(), totalPixelCount.size(), ImGuiInputTextFlags_ReadOnly);
	ImGui::InputText("Total Enrty Count", (char*)totalEntryCount.c_str(), totalEntryCount.size(), ImGuiInputTextFlags_ReadOnly);
	ImGui::InputText("Maximum Entry Count", (char*)maximumEntryCount.c_str(), maximumEntryCount.size(), ImGuiInputTextFlags_ReadOnly);
	ImGui::End();
#endif	

}