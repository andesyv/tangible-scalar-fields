#include "SphereRenderer.h"
#include <globjects/base/File.h>
#include <iostream>
#include <filesystem>
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

void flip(std::vector<unsigned char> & image, uint width, uint height)
{
	unsigned char *imagePtr = &image.front();

	uint rows = height / 2; // Iterate only half the buffer to get a full flip
	uint cols = width * 4;

	std::vector<unsigned char> temp(cols);
	unsigned char *tempPtr = &temp.front();

	for (uint rowIndex = 0; rowIndex < rows; rowIndex++)
	{
		memcpy(tempPtr, imagePtr + rowIndex * cols, cols);
		memcpy(imagePtr + rowIndex * cols, imagePtr + (height - rowIndex - 1) * cols, cols);
		memcpy(imagePtr + (height - rowIndex - 1) * cols, tempPtr, cols);
	}
}

SphereRenderer::SphereRenderer(Viewer* viewer) : Renderer(viewer)
{
	Shader::hintIncludeImplementation(Shader::IncludeImplementation::Fallback);

	Statistics s;

	//m_vertices->setData(viewer->scene()->protein()->atoms(), GL_STATIC_DRAW);
	for (auto i : viewer->scene()->protein()->atoms())
	{
		m_vertices.push_back(Buffer::create());
		m_vertices.back()->setStorage(i, gl::GL_NONE_BIT);
	}
	
	m_elementColorsRadii->setStorage(viewer->scene()->protein()->activeElementColorsRadiiPacked(), gl::GL_NONE_BIT);
	m_residueColors->setStorage(viewer->scene()->protein()->activeResidueColorsPacked(), gl::GL_NONE_BIT);
	m_chainColors->setStorage(viewer->scene()->protein()->activeChainColorsPacked(), gl::GL_NONE_BIT);
	
	m_intersectionBuffer->setStorage(sizeof(vec3) * 1024*1024*128 + sizeof(uint), nullptr, gl::GL_NONE_BIT);
	m_statisticsBuffer->setStorage(sizeof(s), (void*)&s, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);

	m_verticesQuad->setStorage(std::array<vec3, 1>({ vec3(0.0f, 0.0f, 0.0f) }), gl::GL_NONE_BIT);
	auto vertexBindingQuad = m_vaoQuad->binding(0);
	vertexBindingQuad->setBuffer(m_verticesQuad.get(), 0, sizeof(vec3));
	vertexBindingQuad->setFormat(3, GL_FLOAT);
	m_vaoQuad->enable(0);
	m_vaoQuad->unbind();

	m_shaderSourceDefines = StaticStringSource::create("");
	m_shaderDefines = NamedString::create("/defines.glsl", m_shaderSourceDefines.get());

	m_shaderSourceGlobals = File::create("./res/sphere/globals.glsl");
	m_shaderGlobals = NamedString::create("/globals.glsl", m_shaderSourceGlobals.get());

	m_vertexShaderSourceSphere = Shader::sourceFromFile("./res/sphere/sphere-vs.glsl");
	m_geometryShaderSourceSphere = Shader::sourceFromFile("./res/sphere/sphere-gs.glsl");
	m_fragmentShaderSourceSphere = Shader::sourceFromFile("./res/sphere/sphere-fs.glsl");
	m_fragmentShaderSourceSpawn = Shader::sourceFromFile("./res/sphere/spawn-fs.glsl");
	m_vertexShaderSourceImage = Shader::sourceFromFile("./res/sphere/image-vs.glsl");
	m_geometryShaderSourceImage = Shader::sourceFromFile("./res/sphere/image-gs.glsl");
	m_fragmentShaderSourceShade = Shader::sourceFromFile("./res/sphere/shade-fs.glsl");
	m_fragmentShaderSourceAOSample = Shader::sourceFromFile("./res/sphere/aosample-fs.glsl");
	m_fragmentShaderSourceAOBlur = Shader::sourceFromFile("./res/sphere/aoblur-fs.glsl");
	m_fragmentShaderSourceBlend = Shader::sourceFromFile("./res/sphere/blend-fs.glsl");
	
	m_vertexShaderTemplateSphere = Shader::applyGlobalReplacements(m_vertexShaderSourceSphere.get());
	m_geometryShaderTemplateSphere = Shader::applyGlobalReplacements(m_geometryShaderSourceSphere.get());
	m_fragmentShaderTemplateSphere = Shader::applyGlobalReplacements(m_fragmentShaderSourceSphere.get());
	m_fragmentShaderTemplateSpawn = Shader::applyGlobalReplacements(m_fragmentShaderSourceSpawn.get());
	m_vertexShaderTemplateImage = Shader::applyGlobalReplacements(m_vertexShaderSourceImage.get());
	m_geometryShaderTemplateImage = Shader::applyGlobalReplacements(m_geometryShaderSourceImage.get());
	m_fragmentShaderTemplateShade = Shader::applyGlobalReplacements(m_fragmentShaderSourceShade.get());
	m_fragmentShaderTemplateAOSample = Shader::applyGlobalReplacements(m_fragmentShaderSourceAOSample.get());
	m_fragmentShaderTemplateAOBlur = Shader::applyGlobalReplacements(m_fragmentShaderSourceAOBlur.get());
	m_fragmentShaderTemplateBlend = Shader::applyGlobalReplacements(m_fragmentShaderSourceBlend.get());

	m_vertexShaderSphere = Shader::create(GL_VERTEX_SHADER, m_vertexShaderTemplateSphere.get());
	m_geometryShaderSphere = Shader::create(GL_GEOMETRY_SHADER, m_geometryShaderTemplateSphere.get());
	m_fragmentShaderSphere = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateSphere.get());
	m_fragmentShaderSpawn = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateSpawn.get());
	m_vertexShaderImage = Shader::create(GL_VERTEX_SHADER, m_vertexShaderTemplateImage.get());
	m_geometryShaderImage = Shader::create(GL_GEOMETRY_SHADER, m_geometryShaderTemplateImage.get());
	m_fragmentShaderShade = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateShade.get());
	m_fragmentShaderAOSample = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateAOSample.get());
	m_fragmentShaderAOBlur = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateAOBlur.get());
	m_fragmentShaderBlend = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateBlend.get());

	m_programSphere->attach(m_vertexShaderSphere.get(), m_geometryShaderSphere.get(), m_fragmentShaderSphere.get());
	m_programSpawn->attach(m_vertexShaderSphere.get(), m_geometryShaderSphere.get(), m_fragmentShaderSpawn.get());
	m_programShade->attach(m_vertexShaderImage.get(), m_geometryShaderImage.get(), m_fragmentShaderShade.get());
	m_programAOSample->attach(m_vertexShaderImage.get(), m_geometryShaderImage.get(), m_fragmentShaderAOSample.get());
	m_programAOBlur->attach(m_vertexShaderImage.get(), m_geometryShaderImage.get(), m_fragmentShaderAOBlur.get());
	m_programBlend->attach(m_vertexShaderImage.get(), m_geometryShaderImage.get(), m_fragmentShaderBlend.get());


	m_framebufferSize = viewer->viewportSize();

	m_depthTexture = Texture::create(GL_TEXTURE_2D);
	m_depthTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_depthTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_depthTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_depthTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_depthTexture->image2D(0, GL_DEPTH_COMPONENT, m_framebufferSize, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);

	m_spherePositionTexture = Texture::create(GL_TEXTURE_2D);
	m_spherePositionTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_spherePositionTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_spherePositionTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_spherePositionTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_spherePositionTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_sphereNormalTexture = Texture::create(GL_TEXTURE_2D);
	m_sphereNormalTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_sphereNormalTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_sphereNormalTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_sphereNormalTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_sphereNormalTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_surfacePositionTexture = Texture::create(GL_TEXTURE_2D);
	m_surfacePositionTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_surfacePositionTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_surfacePositionTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_surfacePositionTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_surfacePositionTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_surfaceNormalTexture = Texture::create(GL_TEXTURE_2D);
	m_surfaceNormalTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_surfaceNormalTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_surfaceNormalTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_surfaceNormalTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_surfaceNormalTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_sphereDiffuseTexture = Texture::create(GL_TEXTURE_2D);
	m_sphereDiffuseTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_sphereDiffuseTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_sphereDiffuseTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_sphereDiffuseTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_sphereDiffuseTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_surfaceDiffuseTexture = Texture::create(GL_TEXTURE_2D);
	m_surfaceDiffuseTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_surfaceDiffuseTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_surfaceDiffuseTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_surfaceDiffuseTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_surfaceDiffuseTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_ambientTexture = Texture::create(GL_TEXTURE_2D);
	m_ambientTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_ambientTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_ambientTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_ambientTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_ambientTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_blurTexture = Texture::create(GL_TEXTURE_2D);
	m_blurTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_blurTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_blurTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_blurTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_blurTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_blendTexture = Texture::create(GL_TEXTURE_2D);
	m_blendTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_blendTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_blendTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_blendTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_blendTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_offsetTexture = Texture::create(GL_TEXTURE_2D);
	m_offsetTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_offsetTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_offsetTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_offsetTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_offsetTexture->image2D(0, GL_R32UI, m_framebufferSize, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);

	m_environmentTexture = Texture::create(GL_TEXTURE_2D);
	m_environmentTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
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
		m_environmentTexture->generateMipmap();
	}


	m_ssao = std::make_unique<SSAO>();

	for (auto& d : std::filesystem::directory_iterator("./dat/materials"))
	{
		std::filesystem::path materialPath(d);

		if (materialPath.extension().string() == ".png")
		{
			std::vector<unsigned char> materialImage;
			uint materialWidth, materialHeight;

			globjects::debug() << "Loading " << materialPath.string() << " ...";
			uint error = lodepng::decode(materialImage, materialWidth, materialHeight, materialPath.string());
			
			if (error)
			{
				globjects::debug() << "Could not load " << materialPath.string() << "!";
			}
			else
			{
				flip(materialImage, materialWidth, materialHeight);

				auto materialTexture = Texture::create(GL_TEXTURE_2D);
				materialTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				materialTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				materialTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				materialTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				materialTexture->image2D(0, GL_RGBA, materialWidth, materialHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)&materialImage.front());
				//materialTexture->generateMipmap();

				m_materialTextures.push_back(std::move(materialTexture));
			}
		}
	}

	for (auto& d : std::filesystem::directory_iterator("./dat/bumps"))
	{
		std::filesystem::path bumpPath(d);

		if (bumpPath.extension().string() == ".png")
		{
			std::vector<unsigned char> bumpImage;
			uint bumpWidth, bumpHeight;

			globjects::debug() << "Loading " << bumpPath.string() << " ...";
			uint error = lodepng::decode(bumpImage, bumpWidth, bumpHeight, bumpPath.string());

			if (error)
			{
				globjects::debug() << "Could not load " << bumpPath.string() << "!";
			}
			else
			{
				auto bumpTexture = Texture::create(GL_TEXTURE_2D);
				bumpTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				bumpTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				bumpTexture->setParameter(GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
				bumpTexture->setParameter(GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
				bumpTexture->image2D(0, GL_RGBA, bumpWidth, bumpHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)&bumpImage.front());
				bumpTexture->generateMipmap();

				m_bumpTextures.push_back(std::move(bumpTexture));
			}
		}
	}

	m_sphereFramebuffer = Framebuffer::create();
	m_sphereFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_spherePositionTexture.get());
	m_sphereFramebuffer->attachTexture(GL_COLOR_ATTACHMENT1, m_sphereNormalTexture.get());
	m_sphereFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
	m_sphereFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 });
	m_sphereFramebuffer->printStatus();

	m_surfaceFramebuffer = Framebuffer::create();
	m_surfaceFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_surfacePositionTexture.get());
	m_surfaceFramebuffer->attachTexture(GL_COLOR_ATTACHMENT1, m_surfaceNormalTexture.get());
	m_surfaceFramebuffer->attachTexture(GL_COLOR_ATTACHMENT2, m_surfaceDiffuseTexture.get());
	m_surfaceFramebuffer->attachTexture(GL_COLOR_ATTACHMENT3, m_sphereDiffuseTexture.get());
	m_surfaceFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
	m_surfaceFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 });
	m_surfaceFramebuffer->printStatus();

	m_ambientFramebuffer = Framebuffer::create();
	m_ambientFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_ambientTexture.get());
	m_ambientFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0 });
	m_ambientFramebuffer->printStatus();

	m_blurFramebuffer = Framebuffer::create();
	m_blurFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_blurTexture.get());
	m_blurFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0 });
	m_blurFramebuffer->printStatus();
/*
	m_blendFramebuffer = Framebuffer::create();
	m_blendFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_blurTexture.get());
	m_blendFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
	m_blendFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0 });
	m_blendFramebuffer->printStatus();
*/
}

std::list<globjects::File*> SphereRenderer::shaderFiles() const
{
	return std::list<globjects::File*>({ 
		m_shaderSourceGlobals.get(),
		m_vertexShaderSourceSphere.get(),
		m_geometryShaderSourceSphere.get(),
		m_fragmentShaderSourceSphere.get(),
		m_vertexShaderSourceImage.get(),
		m_geometryShaderSourceImage.get(),
		m_fragmentShaderSourceSpawn.get(),
		m_fragmentShaderSourceShade.get(),
		m_fragmentShaderSourceAOSample.get(),
		m_fragmentShaderSourceAOBlur.get(),
		m_fragmentShaderSourceBlend.get()
		});
}

void SphereRenderer::display()
{
	if (viewer()->viewportSize() != m_framebufferSize)
	{
		m_framebufferSize = viewer()->viewportSize();
		m_offsetTexture->image2D(0, GL_R32UI, m_framebufferSize, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);
		m_depthTexture->image2D(0, GL_DEPTH_COMPONENT, m_framebufferSize, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);
		m_spherePositionTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		m_sphereNormalTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		m_surfacePositionTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		m_surfaceNormalTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		m_surfaceDiffuseTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		m_sphereDiffuseTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		m_ambientTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		m_blurTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		m_blendTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	}
	
/*	
	// cyan
	static vec3 ambientMaterial(0.04f, 0.04f, 0.04f);
	static vec3 diffuseMaterial(0.75, 0.75f, 0.75f);
	static vec3 specularMaterial(0.5f, 0.5f, 0.5f);
*/


	// orange
	static vec3 ambientMaterial(0.336f, 0.113f, 0.149f);
	static vec3 diffuseMaterial(1.0f, 0.679f, 0.023f);
	static vec3 specularMaterial(0.707f, 1.0f, 0.997f);




	// nice color scheme
/*
	static vec3 ambientMaterial(0.249f, 0.113f, 0.336f);
	static vec3 diffuseMaterial(0.042f, 0.683f, 0.572f);
	static vec3 specularMaterial(0.629f, 0.629f, 0.629f);
*/
	static float shininess = 20.0f;

	static float sharpness = 1.5f;
	static bool ambientOcclusion = false;
	static bool environmentMapping = false;
	static int coloring = 0;
	static bool animate = false;
	static float animationAmplitude = 1.0f;
	static float animationFrequency = 1.0f;
	static bool lens = false;

	ImGui::Begin("Surface Renderer");

	if (ImGui::CollapsingHeader("Lighting"))
	{
		ImGui::ColorEdit3("Ambient", (float*)&ambientMaterial);
		ImGui::ColorEdit3("Diffuse", (float*)&diffuseMaterial);
		ImGui::ColorEdit3("Specular", (float*)&specularMaterial);
		ImGui::SliderFloat("Shininess", &shininess, 1.0f, 256.0f);
		ImGui::Checkbox("Ambient Occlusion", &ambientOcclusion);
		ImGui::Checkbox("Environment Mapping", &environmentMapping);
	}

	if (ImGui::CollapsingHeader("Surface"))
	{
		ImGui::SliderFloat("Sharpness", &sharpness, 0.5f, 16.0f);
		ImGui::Combo("Coloring", &coloring, "None\0Element\0Residue\0Chain\0");
		ImGui::Checkbox("Magic Lens", &lens);
	}

	static uint materialTextureIndex = 0;

	if (ImGui::CollapsingHeader("Materials"))
	{
		if (ImGui::ListBoxHeader("Material"))
		{
			for (uint i = 0; i< m_materialTextures.size();i++)
			{
				auto& texture = m_materialTextures[i];
				bool selected = (i == materialTextureIndex);
				ImGui::BeginGroup();
				ImGui::PushID(i);
				
				if (ImGui::Selectable("", &selected, 0, ImVec2(0.0f, 32.0f)))
					materialTextureIndex = i;

				ImGui::SameLine();
				ImGui::Image((ImTextureID)texture->id(), ImVec2(32.0f, 32.0f));
				ImGui::PopID();
				ImGui::EndGroup();
			}

			ImGui::ListBoxFooter();
		}
	}

	static uint bumpTextureIndex = 0;

	if (ImGui::CollapsingHeader("Normals"))
	{
		if (ImGui::ListBoxHeader("Normal"))
		{
			for (uint i = 0; i < m_bumpTextures.size(); i++)
			{
				auto& texture = m_bumpTextures[i];
				bool selected = (i == bumpTextureIndex);
				ImGui::BeginGroup();
				ImGui::PushID(i);

				if (ImGui::Selectable("", &selected, 0, ImVec2(0.0f, 32.0f)))
					bumpTextureIndex = i;

				ImGui::SameLine();
				ImGui::Image((ImTextureID)texture->id(), ImVec2(32.0f, 32.0f));
				ImGui::PopID();
				ImGui::EndGroup();
			}

			ImGui::ListBoxFooter();
		}
	}

	if (ImGui::CollapsingHeader("Animation"))
	{
		ImGui::Checkbox("Prodecural Animation", &animate);
		ImGui::SliderFloat("Frequency", &animationFrequency, 1.0f, 32.0f);
		ImGui::SliderFloat("Amplitude", &animationAmplitude, 1.0f, 32.0f);
	}
	
	ImGui::End();
	/*
	if (ImGui::Button("1.0"))
		sharpness = 1.0f;
	ImGui::SameLine();
	if (ImGui::Button("2.0"))
		sharpness = 2.0f;
	ImGui::SameLine();
	if (ImGui::Button("3.0"))
		sharpness = 3.0f;
	ImGui::SameLine();
	if (ImGui::Button("4.0"))
		sharpness = 4.0f;
*/

	ivec2 viewportSize = viewer()->viewportSize();
	double mouseX, mouseY;
	glfwGetCursorPos(viewer()->window(),&mouseX, &mouseY);
	vec2 focusPosition = vec2(2.0f*float(mouseX) / float(viewportSize.x) - 1.0f, -2.0f*float(mouseY) / float(viewportSize.y) + 1.0f);

	uint timestepCount = viewer()->scene()->protein()->atoms().size();
	float animationTime = animate ? float(glfwGetTime()) : -1.0f;

	mat4 viewMatrix = viewer()->viewTransform();
	mat4 inverseViewMatrix = inverse(viewMatrix);
	mat4 modelViewMatrix = viewer()->modelViewTransform();
	mat4 inverseModelViewMatrix = inverse(modelViewMatrix);
	mat4 modelViewProjectionMatrix = viewer()->modelViewProjectionTransform();
	mat4 inverseModelViewProjectionMatrix = inverse(modelViewProjectionMatrix);
	mat4 projectionMatrix = viewer()->projectionTransform();
	mat4 inverseProjectionMatrix = inverse(projectionMatrix);
	mat3 normalMatrix = mat3(transpose(inverseModelViewMatrix));
	mat3 inverseNormalMatrix = inverse(normalMatrix);
	
	vec4 projectionInfo(float(-2.0 / (viewportSize.x * projectionMatrix[0][0])),
		float(-2.0 / (viewportSize.y * projectionMatrix[1][1])),
		float((1.0 - (double)projectionMatrix[0][2]) / projectionMatrix[0][0]),
		float((1.0 + (double)projectionMatrix[1][2]) / projectionMatrix[1][1]));

	float projectionScale = float(viewportSize.y) / fabs(2.0f / projectionMatrix[1][1]);


	const float contributingAtoms = 32.0f;
	float radiusScale = sqrtf(log(contributingAtoms*exp(sharpness)) / sharpness);

	float currentTime = glfwGetTime()*animationFrequency;
	uint currentTimestep = uint(currentTime) % timestepCount;
	uint nextTimestep = (currentTimestep + 1) % timestepCount;
	float animationDelta = currentTime - floor(currentTime);
	int vertexCount = int(viewer()->scene()->protein()->atoms()[currentTimestep].size());

	std::string defines = "";

	if (animate)
		defines += "#define ANIMATION\n";

	if (lens)
		defines += "#define LENSING\n";

	if (coloring > 0)
		defines += "#define COLORING\n";

	if (ambientOcclusion)
		defines += "#define AMBIENT\n";

	if (environmentMapping)
		defines += "#define ENVIRONMENT\n";

	if (defines != m_shaderSourceDefines->string())
	{
		m_shaderSourceDefines->setString(defines);

		for (auto& s : shaderFiles())
			s->reload();
	}

	auto vertexBinding = m_vao->binding(0);
	vertexBinding->setAttribute(0);
	vertexBinding->setBuffer(m_vertices[currentTimestep].get(), 0, sizeof(vec4));
	vertexBinding->setFormat(4, GL_FLOAT);
	m_vao->enable(0);
	
	if (timestepCount > 0)
	{
		auto nextVertexBinding = m_vao->binding(1);
		nextVertexBinding->setAttribute(1);
		nextVertexBinding->setBuffer(m_vertices[nextTimestep].get(), 0, sizeof(vec4));
		nextVertexBinding->setFormat(4, GL_FLOAT);
		m_vao->enable(1);
	}

	m_sphereFramebuffer->bind();
	glClearDepth(1.0f);
	glClearColor(0.0, 0.0, 0.0, 65535.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	m_programSphere->setUniform("modelViewMatrix", modelViewMatrix);
	m_programSphere->setUniform("projectionMatrix", projectionMatrix);
	m_programSphere->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);
	m_programSphere->setUniform("inverseModelViewProjectionMatrix", inverseModelViewProjectionMatrix);
	m_programSphere->setUniform("radiusScale", 1.0f);
	m_programSphere->setUniform("animationDelta", animationDelta);
	m_programSphere->setUniform("animationTime", animationTime);
	m_programSphere->setUniform("animationAmplitude", animationAmplitude);
	m_programSphere->setUniform("animationFrequency", animationFrequency);
	m_programSphere->use();

	m_vao->bind();
	m_vao->drawArrays(GL_POINTS, 0, vertexCount);
	m_vao->unbind();

	m_programSphere->release();

	//m_ssao->display(mat4(1.0f)/*viewer()->modelViewTransform()*/,viewer()->projectionTransform(), m_frameBuffer->id(), m_depthTexture->id(), m_normalTexture->id());

	const uint intersectionClearValue = 1;
	m_intersectionBuffer->clearSubData(GL_R32UI, 0, sizeof(uint), GL_RED_INTEGER, GL_UNSIGNED_INT, &intersectionClearValue);

	const uint offsetClearValue = 0;
	m_offsetTexture->clearImage(0, GL_RED_INTEGER, GL_UNSIGNED_INT, &offsetClearValue);

	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	glDepthFunc(GL_ALWAYS);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_FALSE);

	m_spherePositionTexture->bindActive(0);
	m_offsetTexture->bindImageTexture(0, 0, false, 0, GL_READ_WRITE, GL_R32UI);
	m_elementColorsRadii->bindBase(GL_UNIFORM_BUFFER, 0);
	m_residueColors->bindBase(GL_UNIFORM_BUFFER, 1);
	m_chainColors->bindBase(GL_UNIFORM_BUFFER, 2);
	
	m_programSpawn->setUniform("modelViewMatrix", modelViewMatrix);
	m_programSpawn->setUniform("projectionMatrix", projectionMatrix);
	m_programSpawn->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);
	m_programSpawn->setUniform("inverseModelViewProjectionMatrix", inverseModelViewProjectionMatrix);
	m_programSpawn->setUniform("radiusScale", radiusScale);
	m_programSpawn->setUniform("animationDelta", animationDelta);
	m_programSpawn->setUniform("animationTime", animationTime);
	m_programSpawn->setUniform("animationAmplitude", animationAmplitude);
	m_programSpawn->setUniform("animationFrequency", animationFrequency);
	m_programSpawn->use();

	m_vao->bind();
	m_vao->drawArrays(GL_POINTS, 0, vertexCount);
	m_vao->unbind();

	m_programSpawn->release();

	m_spherePositionTexture->unbindActive(0);
	m_intersectionBuffer->unbind(GL_SHADER_STORAGE_BUFFER);
	m_offsetTexture->unbindImageTexture(0);

	m_sphereFramebuffer->unbind();
	glMemoryBarrier(GL_ALL_BARRIER_BITS);// GL_TEXTURE_FETCH_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

//	uint sphereCount = 0;
//	m_verticesSpawn->getSubData(0, sizeof(uint), &sphereCount);
	//std::array<vec3,1> sphereValues = m_verticesSpawn->getSubData<vec3,1>(16);
	//std::cout << "SphereCount: " << sphereCount << std::endl;
	//std::cout << "SphereValue: " << to_string(sphereValues[0]) << std::endl;

	//std::cout << to_string(inverse(viewer()->projectionTransform())*vec4(0.5, 0.5, 0.0, 1.0)) << std::endl;

	m_surfaceFramebuffer->bind();
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);

	glClearDepth(1.0f);
	glClearColor(0.0, 0.0, 0.0, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//Framebuffer::defaultFBO()->bind();

	m_spherePositionTexture->bindActive(0);
	m_sphereNormalTexture->bindActive(1);
	m_offsetTexture->bindActive(3);
	m_environmentTexture->bindActive(4);
	m_bumpTextures[bumpTextureIndex]->bindActive(5);
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

	m_programShade->setUniform("modelViewMatrix", modelViewMatrix);
	m_programShade->setUniform("projectionMatrix", projectionMatrix);
	m_programShade->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);
	m_programShade->setUniform("inverseModelViewProjectionMatrix", inverseModelViewProjectionMatrix);
	m_programShade->setUniform("normalMatrix", normalMatrix);
	m_programShade->setUniform("lightPosition", vec3(viewer()->worldLightPosition()));
	m_programShade->setUniform("ambientMaterial", ambientMaterial);
	m_programShade->setUniform("diffuseMaterial", diffuseMaterial);
	m_programShade->setUniform("specularMaterial", specularMaterial);
	m_programShade->setUniform("shininess", shininess);
	m_programShade->setUniform("focusPosition", focusPosition);
	m_programShade->setUniform("positionTexture", 0);
	m_programShade->setUniform("normalTexture", 1);
	m_programShade->setUniform("offsetTexture", 3);
	m_programShade->setUniform("environmentTexture", 4);
	m_programShade->setUniform("bumpTexture", 5);
	m_programShade->setUniform("sharpness", sharpness);
	m_programShade->setUniform("coloring", uint(coloring));
	m_programShade->setUniform("environment", environmentMapping);
	m_programShade->setUniform("lens", lens);

	m_programShade->use();
	
	m_vaoQuad->bind();
	m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
	m_vaoQuad->unbind();

	m_programShade->release();

	m_intersectionBuffer->unbind(GL_SHADER_STORAGE_BUFFER);

	m_bumpTextures[bumpTextureIndex]->unbindActive(5);
	m_environmentTexture->unbindActive(4);
	m_offsetTexture->unbindActive(3);
	m_sphereNormalTexture->unbindActive(1);
	m_spherePositionTexture->unbindActive(0);

	m_chainColors->unbind(GL_UNIFORM_BUFFER);
	m_residueColors->unbind(GL_UNIFORM_BUFFER);
	m_elementColorsRadii->unbind(GL_UNIFORM_BUFFER);

	m_surfaceFramebuffer->unbind();

	if (ambientOcclusion)
	{
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		m_ambientFramebuffer->bind();

		m_programAOSample->setUniform("projectionInfo", projectionInfo);
		m_programAOSample->setUniform("projectionScale", projectionScale);
		m_programAOSample->setUniform("surfaceNormalTexture", 0);

		m_surfaceNormalTexture->bindActive(0);
		m_programAOSample->use();

		m_vaoQuad->bind();
		m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
		m_vaoQuad->unbind();

		m_programAOSample->release();
		m_surfaceNormalTexture->unbindActive(0);

		m_ambientFramebuffer->unbind();
		//glMemoryBarrier(GL_ALL_BARRIER_BITS);

		m_blurFramebuffer->bind();
		m_programAOBlur->setUniform("ambientTexture", 0);
		m_programAOBlur->setUniform("offset", vec2(1.0f/float(viewportSize.x),0.0f));
		m_ambientTexture->bindActive(0);
		m_programAOBlur->use();

		m_vaoQuad->bind();
		m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
		m_vaoQuad->unbind();

		m_programAOBlur->release();
		m_ambientTexture->unbindActive(0);
		m_blurFramebuffer->unbind();
		//glMemoryBarrier(GL_ALL_BARRIER_BITS);

		m_ambientFramebuffer->bind();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		m_programAOBlur->setUniform("ambientTexture", 0);
		m_programAOBlur->setUniform("offset", vec2(0.0f,1.0f / float(viewportSize.y)));
		m_blurTexture->bindActive(0);
		m_programAOBlur->use();

		m_vaoQuad->bind();
		m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
		m_vaoQuad->unbind();

		m_programAOBlur->release();
		m_blurTexture->unbindActive(0);
		m_ambientFramebuffer->unbind();
		//glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}
		//m_ssao->display(viewer()->modelViewTransform(), viewer()->projectionTransform(), m_frameBuffers[1]->id(), m_depthTextures[1]->id(), m_normalTextures[1]->id());

	//m_frameBuffers[1]->blit(GL_COLOR_ATTACHMENT0, {0,0,viewer()->viewportSize().x, viewer()->viewportSize().y}, Framebuffer::defaultFBO().get(), GL_BACK, { 0,0,viewer()->viewportSize().x, viewer()->viewportSize().y }, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	//Framebuffer::defaultFBO()->bind();
	glDepthFunc(GL_LEQUAL);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_spherePositionTexture->bindActive(0);
	m_sphereNormalTexture->bindActive(1);
	m_sphereDiffuseTexture->bindActive(2);
	m_surfacePositionTexture->bindActive(3);
	m_surfaceNormalTexture->bindActive(4);
	m_surfaceDiffuseTexture->bindActive(5);
	m_depthTexture->bindActive(6);
	m_ambientTexture->bindActive(7);
	m_materialTextures[materialTextureIndex]->bindActive(8);
	m_environmentTexture->bindActive(9);

	m_programBlend->setUniform("modelViewMatrix", modelViewMatrix);
	m_programBlend->setUniform("projectionMatrix", projectionMatrix);
	m_programBlend->setUniform("modelViewProjection", modelViewProjectionMatrix);
	m_programBlend->setUniform("inverseModelViewProjectionMatrix", inverseModelViewProjectionMatrix);
	m_programBlend->setUniform("normalMatrix", normalMatrix);
	m_programBlend->setUniform("inverseNormalMatrix", inverseNormalMatrix);
	m_programBlend->setUniform("lightPosition", vec3(viewer()->worldLightPosition()));
	m_programBlend->setUniform("ambientMaterial", ambientMaterial);
	m_programBlend->setUniform("diffuseMaterial", diffuseMaterial);
	m_programBlend->setUniform("specularMaterial", specularMaterial);
	m_programBlend->setUniform("shininess", shininess);

	m_programBlend->setUniform("spherePositionTexture", 0);
	m_programBlend->setUniform("sphereNormalTexture", 1);
	m_programBlend->setUniform("sphereDiffuseTexture", 2);

	m_programBlend->setUniform("surfacePositionTexture", 3);
	m_programBlend->setUniform("surfaceNormalTexture", 4);
	m_programBlend->setUniform("surfaceDiffuseTexture", 5);

	m_programBlend->setUniform("depthTexture", 6);

	m_programBlend->setUniform("ambientTexture", 7);
	m_programBlend->setUniform("materialTexture", 8);
	m_programBlend->setUniform("environmentTexture", 9);

	m_programBlend->setUniform("environment", environmentMapping);
	m_programBlend->use();

	m_vaoQuad->bind();
	m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
	m_vaoQuad->unbind();

	m_environmentTexture->unbindActive(9);
	m_materialTextures[materialTextureIndex]->unbindActive(8);
	m_ambientTexture->unbindActive(7);
	m_depthTexture->unbindActive(6);
	m_surfaceDiffuseTexture->unbindActive(5);
	m_surfaceNormalTexture->unbindActive(4);
	m_surfacePositionTexture->unbindActive(3);
	m_sphereDiffuseTexture->unbindActive(2);
	m_sphereNormalTexture->unbindActive(1);
	m_spherePositionTexture->unbindActive(0);

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