#include "SphereRenderer.h"
#include <globjects/base/File.h>
#include <globjects/State.h>
#include <iostream>
#include <filesystem>
#include <imgui.h>
#include "Viewer.h"
#include "Scene.h"
#include "Table.h"
#include <lodepng.h>
#include <sstream>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>


#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#endif

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

struct DepthEntries
{
	uint minDepth = UINT_MAX;
	uint maxDepth = 0;
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

//#define BENCHMARK
//#define STATISTICS

SphereRenderer::SphereRenderer(Viewer* viewer) : Renderer(viewer)
{
	Shader::hintIncludeImplementation(Shader::IncludeImplementation::Fallback);

	Statistics s;
	DepthEntries depthEntries;

	m_intersectionBuffer->setStorage(sizeof(vec3) * 1024*1024*128 + sizeof(uint), nullptr, gl::GL_NONE_BIT);
	m_statisticsBuffer->setStorage(sizeof(s), (void*)&s, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);

	// shader storage buffer object for current depth entries
	m_depthRangeBuffer->setStorage(sizeof(depthEntries), gl::GL_NONE_BIT);

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
	m_fragmentShaderSourceSurface = Shader::sourceFromFile("./res/sphere/surface-fs.glsl");
	m_fragmentShaderSourceAOSample = Shader::sourceFromFile("./res/sphere/aosample-fs.glsl");
	m_fragmentShaderSourceAOBlur = Shader::sourceFromFile("./res/sphere/aoblur-fs.glsl");
	m_fragmentShaderSourceShade = Shader::sourceFromFile("./res/sphere/shade-fs.glsl");
	m_fragmentShaderSourceDOFBlur = Shader::sourceFromFile("./res/sphere/dofblur-fs.glsl");
	m_fragmentShaderSourceDOFBlend = Shader::sourceFromFile("./res/sphere/dofblend-fs.glsl");
	
	m_vertexShaderTemplateSphere = Shader::applyGlobalReplacements(m_vertexShaderSourceSphere.get());
	m_geometryShaderTemplateSphere = Shader::applyGlobalReplacements(m_geometryShaderSourceSphere.get());
	m_fragmentShaderTemplateSphere = Shader::applyGlobalReplacements(m_fragmentShaderSourceSphere.get());
	m_fragmentShaderTemplateSpawn = Shader::applyGlobalReplacements(m_fragmentShaderSourceSpawn.get());
	m_vertexShaderTemplateImage = Shader::applyGlobalReplacements(m_vertexShaderSourceImage.get());
	m_geometryShaderTemplateImage = Shader::applyGlobalReplacements(m_geometryShaderSourceImage.get());
	m_fragmentShaderTemplateSurface = Shader::applyGlobalReplacements(m_fragmentShaderSourceSurface.get());
	m_fragmentShaderTemplateAOSample = Shader::applyGlobalReplacements(m_fragmentShaderSourceAOSample.get());
	m_fragmentShaderTemplateAOBlur = Shader::applyGlobalReplacements(m_fragmentShaderSourceAOBlur.get());
	m_fragmentShaderTemplateShade = Shader::applyGlobalReplacements(m_fragmentShaderSourceShade.get());
	m_fragmentShaderTemplateDOFBlur = Shader::applyGlobalReplacements(m_fragmentShaderSourceDOFBlur.get());
	m_fragmentShaderTemplateDOFBlend = Shader::applyGlobalReplacements(m_fragmentShaderSourceDOFBlend.get());

	m_vertexShaderSphere = Shader::create(GL_VERTEX_SHADER, m_vertexShaderTemplateSphere.get());
	m_geometryShaderSphere = Shader::create(GL_GEOMETRY_SHADER, m_geometryShaderTemplateSphere.get());
	m_fragmentShaderSphere = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateSphere.get());
	m_fragmentShaderSpawn = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateSpawn.get());
	m_vertexShaderImage = Shader::create(GL_VERTEX_SHADER, m_vertexShaderTemplateImage.get());
	m_geometryShaderImage = Shader::create(GL_GEOMETRY_SHADER, m_geometryShaderTemplateImage.get());
	m_fragmentShaderSurface = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateSurface.get());
	m_fragmentShaderAOSample = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateAOSample.get());
	m_fragmentShaderAOBlur = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateAOBlur.get());
	m_fragmentShaderShade = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateShade.get());
	m_fragmentShaderDOFBlur = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateDOFBlur.get());
	m_fragmentShaderDOFBlend = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateDOFBlend.get());

	m_programSphere->attach(m_vertexShaderSphere.get(), m_geometryShaderSphere.get(), m_fragmentShaderSphere.get());
	m_programSpawn->attach(m_vertexShaderSphere.get(), m_geometryShaderSphere.get(), m_fragmentShaderSpawn.get());
	m_programSurface->attach(m_vertexShaderImage.get(), m_geometryShaderImage.get(), m_fragmentShaderSurface.get());
	m_programAOSample->attach(m_vertexShaderImage.get(), m_geometryShaderImage.get(), m_fragmentShaderAOSample.get());
	m_programAOBlur->attach(m_vertexShaderImage.get(), m_geometryShaderImage.get(), m_fragmentShaderAOBlur.get());
	m_programShade->attach(m_vertexShaderImage.get(), m_geometryShaderImage.get(), m_fragmentShaderShade.get());
	m_programDOFBlur->attach(m_vertexShaderImage.get(), m_geometryShaderImage.get(), m_fragmentShaderDOFBlur.get());
	m_programDOFBlend->attach(m_vertexShaderImage.get(), m_geometryShaderImage.get(), m_fragmentShaderDOFBlend.get());

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

	m_colorTexture = Texture::create(GL_TEXTURE_2D);
	m_colorTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_colorTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_colorTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_colorTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_colorTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

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

	m_colorMapTexture = Texture::create(GL_TEXTURE_1D);
	m_colorMapTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_colorMapTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_colorMapTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_colorMapTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

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
				bumpTexture->setParameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
				bumpTexture->setParameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
				bumpTexture->image2D(0, GL_RGBA, bumpWidth, bumpHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)&bumpImage.front());
				bumpTexture->generateMipmap();

				m_bumpTextures.push_back(std::move(bumpTexture));
			}
		}
	}

	for (auto& d : std::filesystem::directory_iterator("./dat"))
	{
		std::filesystem::path csvPath(d);

		if (csvPath.extension().string() == ".csv")
		{
			// log CSV files that were found in the "./dat" folder
			globjects::debug() << "Found CSV file: " << csvPath.string() << " ...";
			std::string filename = csvPath.filename().string();

			// update collections containing all current CSV files
			m_guiFileNames += filename + '\0';
			m_fileNames.push_back(filename);
		}
	}

	m_sphereFramebuffer = Framebuffer::create();
	m_sphereFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_spherePositionTexture.get());
	m_sphereFramebuffer->attachTexture(GL_COLOR_ATTACHMENT1, m_sphereNormalTexture.get());
	m_sphereFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
	m_sphereFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 });

	m_surfaceFramebuffer = Framebuffer::create();
	m_surfaceFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_surfacePositionTexture.get());
	m_surfaceFramebuffer->attachTexture(GL_COLOR_ATTACHMENT1, m_surfaceNormalTexture.get());
	m_surfaceFramebuffer->attachTexture(GL_COLOR_ATTACHMENT2, m_surfaceDiffuseTexture.get());
	m_surfaceFramebuffer->attachTexture(GL_COLOR_ATTACHMENT3, m_sphereDiffuseTexture.get());
	m_surfaceFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
	m_surfaceFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 });

	m_shadeFramebuffer = Framebuffer::create();
	m_shadeFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_colorTexture.get());
	m_shadeFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0 });
	m_shadeFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());

	m_aoBlurFramebuffer = Framebuffer::create();
	m_aoBlurFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_blurTexture.get());
	m_aoBlurFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0 });

	m_aoFramebuffer = Framebuffer::create();
	m_aoFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_ambientTexture.get());
	m_aoFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0 });

	m_dofBlurFramebuffer = Framebuffer::create();
	m_dofBlurFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_sphereNormalTexture.get());
	m_dofBlurFramebuffer->attachTexture(GL_COLOR_ATTACHMENT1, m_surfaceNormalTexture.get());
	m_dofBlurFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 });

	m_dofFramebuffer = Framebuffer::create();
	m_dofFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_sphereDiffuseTexture.get());
	m_dofFramebuffer->attachTexture(GL_COLOR_ATTACHMENT1, m_surfaceDiffuseTexture.get());
	m_dofFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 });

#ifdef BENCHMARK
	m_sphereQuery = Query::create();
	m_spawnQuery = Query::create();
	m_surfaceQuery = Query::create();
	m_shadeQuery = Query::create();
#endif

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
		m_fragmentShaderSourceSurface.get(),
		m_fragmentShaderSourceAOSample.get(),
		m_fragmentShaderSourceAOBlur.get(),
		m_fragmentShaderSourceShade.get(),
		m_fragmentShaderSourceDOFBlur.get(),
		m_fragmentShaderSourceDOFBlend.get()
		});
}

void SphereRenderer::display()
{
#ifdef BENCHMARK

	static mat4 viewTransform = viewer()->viewTransform();
	static mat4 inverseViewTransform = inverse(viewTransform);

	uint axisIndex = (m_benchmarkCounter / 360) % 3;
	vec4 baseAxis = vec4(0.0);
	baseAxis[axisIndex] = 1.0;

	vec4 transformedAxis = inverseViewTransform * baseAxis;

	float angle = float(m_benchmarkCounter % 360);
	mat4 newViewTransform = rotate(viewTransform, angle * pi<float>() / 180.0f, vec3(transformedAxis));
	viewer()->setViewTransform(newViewTransform);

	std::array<ivec2,3> windowSizes = { ivec2(768,768), ivec2(1024,1024), ivec2(1280,1280) };
	uint windowIndex = m_benchmarkPhase % windowSizes.size();
	glfwSetWindowSize(viewer()->window(), windowSizes[windowIndex].x, windowSizes[windowIndex].y);

	if (m_benchmarkPhase == 0 && m_benchmarkIndex == 0 && m_benchmarkCounter == 0 && m_benchmarkWarmup == true)
	{
		std::filesystem::path filePath(viewer()->scene()->table()->filename());
		std::string pdbName = filePath.stem().string();
		std::transform(pdbName.begin(), pdbName.end(), pdbName.begin(), ::toupper);

		std::cout << pdbName << ",";
		std::cout << viewer()->scene()->table()->atoms().at(0).size();
	}

	if (m_benchmarkPhase >= windowSizes.size())
	{
		std::cout << std::endl;
		exit(0);
	}
	m_benchmarkCounter++;

#endif

	auto currentState = State::currentState();

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
		m_colorTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	}
	
	ivec2 viewportSize = viewer()->viewportSize();
	double mouseX, mouseY;
	glfwGetCursorPos(viewer()->window(), &mouseX, &mouseY);
	vec2 focusPosition = vec2(2.0f*float(mouseX) / float(viewportSize.x) - 1.0f, -2.0f*float(mouseY) / float(viewportSize.y) + 1.0f);

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

	// white
	static vec3 ambientMaterial(0.3f, 0.3f, 0.3f);
	static vec3 diffuseMaterial(0.6f, 0.6f, 0.6f);
	static vec3 specularMaterial(0.3f, 0.3f, 0.3f);

	//std::cout << to_string(projectionInfo) << std::endl

	vec4 nearPlane = inverseProjectionMatrix * vec4(0.0, 0.0, -1.0, 1.0);
	nearPlane /= nearPlane.w;

/*	
	// cyan
	static vec3 ambientMaterial(0.04f, 0.04f, 0.04f);
	static vec3 diffuseMaterial(0.75, 0.75f, 0.75f);
	static vec3 specularMaterial(0.5f, 0.5f, 0.5f);
*/


	// orange
/*
	static vec3 ambientMaterial(0.336f, 0.113f, 0.149f);
	static vec3 diffuseMaterial(1.0f, 0.679f, 0.023f);
	static vec3 specularMaterial(0.707f, 1.0f, 0.997f);
*/

	// nice color scheme
/*
	static vec3 ambientMaterial(0.249f, 0.113f, 0.336f);
	static vec3 diffuseMaterial(0.042f, 0.683f, 0.572f);
	static vec3 specularMaterial(0.629f, 0.629f, 0.629f);
*/
	static float shininess = 20.0f;

	static float sharpness = 1.0f;
	static float distanceBlending = 0.0f;
	static float distanceScale = 1.0;
	static bool ambientOcclusion = false;
	static bool environmentMapping = false;
	static bool normalMapping = false;
	static bool materialMapping = false;
	static bool depthOfField = false;

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
		ImGui::Checkbox("Ambient Occlusion Enabled", &ambientOcclusion);
		ImGui::Checkbox("Material Mapping Enabled", &materialMapping);
		ImGui::Checkbox("Normal Mapping Enabled", &normalMapping);
		ImGui::Checkbox("Environment Mapping Enabled", &environmentMapping);
		ImGui::Checkbox("Depth of Field Enabled", &depthOfField);
	}

	if (ImGui::CollapsingHeader("Surface"))
	{		
		ImGui::SliderFloat("Sharpness", &sharpness, 0.5f, 16.0f);
		/*
		ImGui::SameLine();
		if (ImGui::Button("1.0"))
			sharpness = 1.0;
		ImGui::SameLine();
		if (ImGui::Button("2.0"))
			sharpness = 2.0;
		ImGui::SameLine();
		if (ImGui::Button("3.0"))
			sharpness = 3.0;
		ImGui::SameLine();
		if (ImGui::Button("4.0"))
			sharpness = 4.0;
		*/
		ImGui::SliderFloat("Dist. Blending", &distanceBlending, 0.0f, 1.0f);
		ImGui::SliderFloat("Dist. Scale", &distanceScale, 0.0f, 16.0f);
		ImGui::Combo("Coloring", &coloring, "None\0Element\0Residue\0Chain\0");
		ImGui::Checkbox("Magic Lens", &lens);
	}

#ifdef BENCHMARK
	std::array<float, 4> sharpnessValues = { 1.0f, 2.0f, 3.0f, 4.0f };
	sharpness = sharpnessValues[m_benchmarkIndex % sharpnessValues.size()];
#endif
	
	static uint materialTextureIndex = 0;

	if (materialMapping)
	{
		if (ImGui::CollapsingHeader("Material Mapping"))
		{
			if (ImGui::ListBoxHeader("Material Map"))
			{
				for (uint i = 0; i < m_materialTextures.size(); i++)
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
	}
		
	static uint bumpTextureIndex = 0;

	if (normalMapping)
	{
		if (ImGui::CollapsingHeader("Normal Mapping"))
		{
			if (ImGui::ListBoxHeader("Normal Map"))
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
	}
	

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
	float fieldOfView = 2.0f * atan(1.0f / projectionMatrix[1][1]);
	//std::cout << "FOV :" << fieldOfView << std::endl;

	static float focalDistance = 2.0f*sqrt(3.0f);
	static float maximumCoCRadius = 9.0f;
	static float farRadiusRescale = 1.0f;
	static float focalLength = 1.0f;
	static float aparture = 1.0f;
	static float fStop = 1.0;
	static int fStop_current = 12;

	const char* fStops[] = { "0.7", "0.8", "1.0", "1.2", "1.4", "1.7", "2.0", "2.4", "2.8", "3.3", "4.0", "4.8", "5.6", "6.7", "8.0", "9.5", "11.0", "16.0", "22.0", "32.0" };

	if (depthOfField)
	{
		if (ImGui::CollapsingHeader("Depth of Field"))
		{
			ImGui::SliderFloat("Focal Distance", &focalDistance, 0.1f, 35.0f);
			ImGui::Combo("F-stop", &fStop_current, fStops, IM_ARRAYSIZE(fStops));

			ImGui::SliderFloat("Max. CoC Radius", &maximumCoCRadius, 1.0f, 20.0f);
			ImGui::SliderFloat("Far Radius Scale", &farRadiusRescale, 0.1f, 5.0f);

		}
	}

	fStop = std::stof(fStops[fStop_current]);
	focalLength = 1.0f / (tan(fieldOfView * 0.5f) * 2.0f);
	aparture = focalLength / fStop;

	if (ImGui::CollapsingHeader("Animation"))
	{
		ImGui::Checkbox("Prodecural Animation", &animate);
		ImGui::SliderFloat("Frequency", &animationFrequency, 1.0f, 256.0f);
		ImGui::SliderFloat("Amplitude", &animationAmplitude, 1.0f, 32.0f);
	}

	ImGui::End();

	// Scatterplot GUI --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

	ImGui::Begin("Scatterplot");

	if (ImGui::CollapsingHeader("CSV-Files"))
	{

		ImGui::Combo("Files", &m_fileDataID, m_guiFileNames.c_str());

		if (ImGui::Button("Select"))
		{
			//std::cout << "File selection event - " << "File: " << m_fileDataID << "\n";

			// reset variables --------------------------------------------------
			m_guiColumnNames = { 'N', 'o', 'n', 'e', '\0' };

			// reset selection of ImGui combo selection
			m_xAxisDataID = 0; 
			m_yAxisDataID = 0; 
			m_radiusDataID = 0; 
			m_colorDataID = 0;

			// ------------------------------------------------------------------

			if (m_fileDataID != 0) {
			
				// initialize table
				viewer()->scene()->table()->load("./dat/" + m_fileNames[m_fileDataID]);

				// extract column names and prepare GUI
				std::vector<std::string> tempNames = viewer()->scene()->table()->getColumnNames();

				for (std::vector<std::string>::iterator it = tempNames.begin(); it != tempNames.end(); ++it) {
					m_guiColumnNames += *it + '\0';
				}
			}
		}

	}

	if (ImGui::CollapsingHeader("Column Data"))
	{

		// show all column names from selected CSV file
		ImGui::Combo("X-axis", &m_xAxisDataID, m_guiColumnNames.c_str());
		ImGui::Combo("Y-axis", &m_yAxisDataID, m_guiColumnNames.c_str());
		ImGui::Combo("Radius", &m_radiusDataID, m_guiColumnNames.c_str());
		ImGui::Combo("Color", &m_colorDataID, m_guiColumnNames.c_str());

		ImGui::SliderFloat("Radius-Scale", &m_radiusMultiplier, 0.0f, 10.0f);

		if (ImGui::Button("Load"))
		{

			// update buffers according to recent changes -> since combo also contains 'None" we need to subtract 1 from ID
			viewer()->scene()->table()->updateBuffers(m_xAxisDataID-1, m_yAxisDataID-1, m_radiusDataID-1, m_colorDataID-1);

			// update VBOs for all four columns
			m_xColumnBuffer->setData(viewer()->scene()->table()->activeXColumn(), GL_STATIC_DRAW);
			m_yColumnBuffer->setData(viewer()->scene()->table()->activeYColumn(), GL_STATIC_DRAW);
			m_radiusColumnBuffer->setData(viewer()->scene()->table()->activeRadiusColumn(), GL_STATIC_DRAW);
			m_colorColumnBuffer->setData(viewer()->scene()->table()->activeColorColumn(), GL_STATIC_DRAW);

			// update VAO for all buffers ----------------------------------------------------
			auto vertexBinding = m_vao->binding(0);
			
			vertexBinding->setBuffer(m_xColumnBuffer.get(), 0, sizeof(float));
			vertexBinding->setFormat(1, GL_FLOAT);
			m_vao->enable(0);

			vertexBinding = m_vao->binding(1);
			vertexBinding->setBuffer(m_yColumnBuffer.get(), 0, sizeof(float));
			vertexBinding->setFormat(1, GL_FLOAT);
			m_vao->enable(1);

			vertexBinding = m_vao->binding(2);
			vertexBinding->setBuffer(m_radiusColumnBuffer.get(), 0, sizeof(float));
			vertexBinding->setFormat(1, GL_FLOAT);
			m_vao->enable(2);

			vertexBinding = m_vao->binding(3);
			vertexBinding->setBuffer(m_colorColumnBuffer.get(), 0, sizeof(float));
			vertexBinding->setFormat(1, GL_FLOAT);
			m_vao->enable(3);
			// -------------------------------------------------------------------------------


			// Scaling the model's bounding box to the canonical view volume
			vec3 boundingBoxSize = viewer()->scene()->table()->maximumBounds() - viewer()->scene()->table()->minimumBounds();
			float maximumSize = std::max({ boundingBoxSize.x, boundingBoxSize.y, boundingBoxSize.z });
			mat4 modelTransform = scale(vec3(2.0f) / vec3(maximumSize));
			modelTransform = modelTransform * translate(-0.5f*(viewer()->scene()->table()->minimumBounds() + viewer()->scene()->table()->maximumBounds()));
			viewer()->setModelTransform(modelTransform);
		}

	}

	if (ImGui::CollapsingHeader("Color Maps"))
	{
		// show all available color-maps
		ImGui::Combo("Maps", &m_colorMap, "None\0Cubehelix\0GistEart\0GnuPlot2\0Magma\0Virdis\0YlGnBu\0");

		if (ImGui::Button("Apply"))
		{

			if (m_colorMap > 0) {
				std::vector<std::string> colorMapFilenames = { "./dat/colormaps/cubehelix_1D.png", "./dat/colormaps/gist_earth_1D.png",  "./dat/colormaps/gnuplot2_1D.png" , "./dat/colormaps/magma_1D.png", "./dat/colormaps/virdis_1D.png", "./dat/colormaps/YlGnBu_1D.png" };

				uint colorMapWidth, colorMapHeight;
				std::vector<unsigned char> colorMapImage;
				uint error = lodepng::decode(colorMapImage, colorMapWidth, colorMapHeight, colorMapFilenames[m_colorMap - 1]);

				if (error)
					globjects::debug() << "Could not load " << colorMapFilenames[m_colorMap - 1] << "!";
				else
				{
					m_colorMapTexture->image1D(0, GL_RGBA, colorMapWidth, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)&colorMapImage.front());
					m_colorMapTexture->generateMipmap();

					// store width of texture and mark as loaded
					m_ColorMapWidth = colorMapWidth;
					m_colorMapLoaded = true;
				}
			}
			else 
			{
				// disable color map
				m_colorMapLoaded = false;
			}
		}

		ImGui::Checkbox("Surface Illumination", &m_surfaceIllumination);

	}

	ImGui::End();

	// ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

	// do not render if data is not loaded
	if (viewer()->scene()->table()->activeTableData().size() == 0)
	{
		return;
	}

	const float contributingAtoms = 128.0f;
	float radiusScale = sqrtf(log(contributingAtoms*exp(sharpness)) / sharpness);

	uint timestepCount = viewer()->scene()->table()->activeTableData().size();
	float animationTime = animate ? float(glfwGetTime()) : -1.0f;

	float currentTime =  glfwGetTime()*animationFrequency;
	uint currentTimestep =  uint(currentTime) % timestepCount;
	uint nextTimestep = (currentTimestep + 1) % timestepCount;
	float animationDelta = currentTime - floor(currentTime);
	int vertexCount = int(viewer()->scene()->table()->activeTableData()[currentTimestep].size());
	//std::cout << currentTimestep << std::endl;
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

	if (normalMapping)
		defines += "#define NORMAL\n";

	if (materialMapping)
		defines += "#define MATERIAL\n";

	if (depthOfField)
		defines += "#define DEPTHOFFIELD\n";

	if (m_colorMapLoaded)
		defines += "#define COLORMAP\n";

	if(m_surfaceIllumination)
		defines += "#define ILLUMINATION\n";

	if (defines != m_shaderSourceDefines->string())
	{
		m_shaderSourceDefines->setString(defines);

		for (auto& s : shaderFiles())
			s->reload();
	}

#ifdef BENCHMARK
	m_sphereQuery->begin(GL_TIME_ELAPSED);
#endif

	m_sphereFramebuffer->bind();
	glClearDepth(1.0f);
	glClearColor(0.0, 0.0, 0.0, 65535.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	//glEnable(GL_DEPTH_CLAMP);


	//std::cout << to_string(inverseProjectionMatrix * vec4(0.0, 0.0, -1.0, 1.0)) << std::endl;

	m_programSphere->setUniform("modelViewMatrix", modelViewMatrix);
	m_programSphere->setUniform("projectionMatrix", projectionMatrix);
	m_programSphere->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);
	m_programSphere->setUniform("inverseModelViewProjectionMatrix", inverseModelViewProjectionMatrix);
	m_programSphere->setUniform("radiusScale", 1.0f);
	m_programSphere->setUniform("clipRadiusScale", radiusScale);
	m_programSphere->setUniform("nearPlaneZ", nearPlane.z);
	m_programSphere->setUniform("animationDelta", animationDelta);
	m_programSphere->setUniform("animationTime", animationTime);
	m_programSphere->setUniform("animationAmplitude", animationAmplitude);
	m_programSphere->setUniform("animationFrequency", animationFrequency);
	m_programSphere->setUniform("radiusMultiplier", m_radiusMultiplier);
	m_programSphere->use();

	m_vao->bind();
	m_vao->drawArrays(GL_POINTS, 0, vertexCount);
	m_vao->unbind();

	m_programSphere->release();

#ifdef BENCHMARK
	m_sphereQuery->end(GL_TIME_ELAPSED);
#endif

#ifdef BENCHMARK
	m_spawnQuery->begin(GL_TIME_ELAPSED);
#endif

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
	
	m_programSpawn->setUniform("modelViewMatrix", modelViewMatrix);
	m_programSpawn->setUniform("projectionMatrix", projectionMatrix);
	m_programSpawn->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);
	m_programSpawn->setUniform("inverseModelViewProjectionMatrix", inverseModelViewProjectionMatrix);
	m_programSpawn->setUniform("radiusScale", radiusScale);
	m_programSpawn->setUniform("clipRadiusScale", radiusScale);
	m_programSpawn->setUniform("nearPlaneZ", nearPlane.z);
	m_programSpawn->setUniform("animationDelta", animationDelta);
	m_programSpawn->setUniform("animationTime", animationTime);
	m_programSpawn->setUniform("animationAmplitude", animationAmplitude);
	m_programSpawn->setUniform("animationFrequency", animationFrequency);
	m_programSpawn->setUniform("radiusMultiplier", m_radiusMultiplier);
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

#ifdef BENCHMARK
	m_spawnQuery->end(GL_TIME_ELAPSED);
#endif

#ifdef BENCHMARK
	m_surfaceQuery->begin(GL_TIME_ELAPSED);
#endif

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
	glClearColor(0.0f, 0.0f, 0.0f, 65535.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//Framebuffer::defaultFBO()->bind();

	m_spherePositionTexture->bindActive(0);
	m_sphereNormalTexture->bindActive(1);
	m_offsetTexture->bindActive(3);
	m_environmentTexture->bindActive(4);
	m_bumpTextures[bumpTextureIndex]->bindActive(5);
	m_materialTextures[materialTextureIndex]->bindActive(6);

	// SSBO
	m_intersectionBuffer->bindBase(GL_SHADER_STORAGE_BUFFER, 1);
	m_statisticsBuffer->bindBase(GL_SHADER_STORAGE_BUFFER, 2);
	m_depthRangeBuffer->bindBase(GL_SHADER_STORAGE_BUFFER, 3);

	const uint depthMinClearValue = UINT_MAX;
	const uint depthMaxClearValue = 0;
	m_depthRangeBuffer->clearSubData(GL_R32UI, 0, sizeof(uint), GL_RED_INTEGER, GL_UNSIGNED_INT, &depthMinClearValue);
	m_depthRangeBuffer->clearSubData(GL_R32UI, 1 * sizeof(uint), sizeof(uint), GL_RED_INTEGER, GL_UNSIGNED_INT, &depthMaxClearValue);

//	std::cout << to_string(viewer()->worldLightPosition()) << std::endl;
//	std::cout << "DIST: " << length(viewer()->worldLightPosition()) << std::endl;


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

	m_programSurface->setUniform("modelViewMatrix", modelViewMatrix);
	m_programSurface->setUniform("projectionMatrix", projectionMatrix);
	m_programSurface->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);
	m_programSurface->setUniform("inverseModelViewProjectionMatrix", inverseModelViewProjectionMatrix);
	m_programSurface->setUniform("normalMatrix", normalMatrix);
	m_programSurface->setUniform("lightPosition", vec3(viewer()->worldLightPosition()));
	m_programSurface->setUniform("ambientMaterial", ambientMaterial);
	m_programSurface->setUniform("diffuseMaterial", diffuseMaterial);
	m_programSurface->setUniform("specularMaterial", specularMaterial);
	m_programSurface->setUniform("shininess", shininess);
	m_programSurface->setUniform("focusPosition", focusPosition);
	m_programSurface->setUniform("positionTexture", 0);
	m_programSurface->setUniform("normalTexture", 1);
	m_programSurface->setUniform("offsetTexture", 3);
	m_programSurface->setUniform("environmentTexture", 4);
	m_programSurface->setUniform("bumpTexture", 5);
	m_programSurface->setUniform("materialTexture", 6);
	m_programSurface->setUniform("sharpness", sharpness);
	m_programSurface->setUniform("coloring", uint(coloring));
	m_programSurface->setUniform("environment", environmentMapping);
	m_programSurface->setUniform("lens", lens);

	m_programSurface->use();
	
	m_vaoQuad->bind();
	m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
	m_vaoQuad->unbind();

	m_programSurface->release();

	m_intersectionBuffer->unbind(GL_SHADER_STORAGE_BUFFER);
	//m_depthRangeBuffer->unbind(GL_SHADER_STORAGE_BUFFER);

	m_materialTextures[materialTextureIndex]->unbindActive(6);
	m_bumpTextures[bumpTextureIndex]->unbindActive(5);
	m_environmentTexture->unbindActive(4);
	m_offsetTexture->unbindActive(3);
	m_sphereNormalTexture->unbindActive(1);
	m_spherePositionTexture->unbindActive(0);

	m_surfaceFramebuffer->unbind();

#ifdef BENCHMARK
	m_surfaceQuery->end(GL_TIME_ELAPSED);
#endif

#ifdef BENCHMARK
	m_shadeQuery->begin(GL_TIME_ELAPSED);
#endif

	if (ambientOcclusion)
	{
		//glMemoryBarrier(GL_ALL_BARRIER_BITS);
		m_aoFramebuffer->bind();

		m_programAOSample->setUniform("projectionInfo", projectionInfo);
		m_programAOSample->setUniform("projectionScale", projectionScale);
		m_programAOSample->setUniform("viewLightPosition", modelViewMatrix*viewer()->worldLightPosition());
		m_programAOSample->setUniform("surfaceNormalTexture", 0);

		m_surfaceNormalTexture->bindActive(0);
		m_programAOSample->use();

		m_vaoQuad->bind();
		m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
		m_vaoQuad->unbind();

		m_programAOSample->release();

		m_aoFramebuffer->unbind();
		//glMemoryBarrier(GL_ALL_BARRIER_BITS);

		m_aoBlurFramebuffer->bind();
		m_programAOBlur->setUniform("normalTexture", 0);
		m_programAOBlur->setUniform("ambientTexture", 1);
		m_programAOBlur->setUniform("offset", vec2(1.0f/float(viewportSize.x),0.0f));

		m_ambientTexture->bindActive(1);
		m_programAOBlur->use();

		m_vaoQuad->bind();
		m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
		m_vaoQuad->unbind();

		m_programAOBlur->release();

		m_ambientTexture->unbindActive(1);
		m_aoBlurFramebuffer->unbind();
		//glMemoryBarrier(GL_ALL_BARRIER_BITS);

		m_aoFramebuffer->bind();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		m_programAOBlur->setUniform("offset", vec2(0.0f,1.0f / float(viewportSize.y)));

		m_blurTexture->bindActive(1);
		m_programAOBlur->use();

		m_vaoQuad->bind();
		m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
		m_vaoQuad->unbind();

		m_programAOBlur->release();

		m_blurTexture->unbindActive(1);
		m_surfaceNormalTexture->unbindActive(0);

		m_aoFramebuffer->unbind();
		//glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	//m_ssao->display(viewer()->modelViewTransform(), viewer()->projectionTransform(), m_frameBuffers[1]->id(), m_depthTextures[1]->id(), m_normalTextures[1]->id());

	//m_frameBuffers[1]->blit(GL_COLOR_ATTACHMENT0, {0,0,viewer()->viewportSize().x, viewer()->viewportSize().y}, Framebuffer::defaultFBO().get(), GL_BACK, { 0,0,viewer()->viewportSize().x, viewer()->viewportSize().y }, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	//Framebuffer::defaultFBO()->bind();
	//glDepthFunc(GL_LEQUAL);

	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	m_shadeFramebuffer->bind();
	glDepthMask(GL_FALSE);

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

	//m_depthRangeBuffer->bindBase(GL_SHADER_STORAGE_BUFFER, 3);

	m_programShade->setUniform("modelViewMatrix", modelViewMatrix);
	m_programShade->setUniform("projectionMatrix", projectionMatrix);
	m_programShade->setUniform("modelViewProjection", modelViewProjectionMatrix);
	m_programShade->setUniform("inverseModelViewProjectionMatrix", inverseModelViewProjectionMatrix);
	m_programShade->setUniform("normalMatrix", normalMatrix);
	m_programShade->setUniform("inverseNormalMatrix", inverseNormalMatrix);
	m_programShade->setUniform("lightPosition", vec3(viewer()->worldLightPosition()));
	m_programShade->setUniform("ambientMaterial", ambientMaterial);
	m_programShade->setUniform("diffuseMaterial", diffuseMaterial);
	m_programShade->setUniform("specularMaterial", specularMaterial);
	m_programShade->setUniform("distanceBlending", distanceBlending);
	m_programShade->setUniform("distanceScale", distanceScale);
	m_programShade->setUniform("shininess", shininess);
	m_programShade->setUniform("focusPosition", focusPosition);

	m_programShade->setUniform("spherePositionTexture", 0);
	m_programShade->setUniform("sphereNormalTexture", 1);
	m_programShade->setUniform("sphereDiffuseTexture", 2);

	m_programShade->setUniform("surfacePositionTexture", 3);
	m_programShade->setUniform("surfaceNormalTexture", 4);
	m_programShade->setUniform("surfaceDiffuseTexture", 5);

	m_programShade->setUniform("depthTexture", 6);

	m_programShade->setUniform("ambientTexture", 7);
	m_programShade->setUniform("materialTexture", 8);
	m_programShade->setUniform("environmentTexture", 9);

	m_programShade->setUniform("environment", environmentMapping);

	if (m_colorMapLoaded)
	{
		m_colorMapTexture->bindActive(10);
		m_programShade->setUniform("colorMapTexture", 10);
		m_programShade->setUniform("textureWidth", m_ColorMapWidth);
	}

	/*
	std::cout << "maximumCoCRadius: " << maximumCoCRadius << std::endl;
	std::cout << "aparture: " << aparture << std::endl;
	std::cout << "focalDistance: " << focalDistance << std::endl;
	std::cout << "focalLength: " << focalLength << std::endl << std::endl;
	*/
	m_programShade->setUniform("maximumCoCRadius", maximumCoCRadius);
	m_programShade->setUniform("aparture", aparture);
	m_programShade->setUniform("focalDistance", focalDistance);
	m_programShade->setUniform("focalLength", focalLength);

	m_programShade->use();

	m_vaoQuad->bind();
	m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
	m_vaoQuad->unbind();

	m_depthRangeBuffer->unbind(GL_SHADER_STORAGE_BUFFER);

	if (m_colorMapLoaded)
	{
		m_colorMapTexture->unbindActive(10);
	}

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

	//glDisable(GL_BLEND);
	m_programShade->release();
	m_shadeFramebuffer->unbind();

	if (depthOfField)
	{
		m_dofBlurFramebuffer->bind();

		m_colorTexture->bindActive(0);
		m_colorTexture->bindActive(1);
		
		m_programDOFBlur->setUniform("maximumCoCRadius", maximumCoCRadius);
		m_programDOFBlur->setUniform("aparture", aparture);
		m_programDOFBlur->setUniform("focalDistance", focalDistance);
		m_programDOFBlur->setUniform("focalLength", focalLength);

		m_programDOFBlur->setUniform("uMaxCoCRadiusPixels", (int)round(maximumCoCRadius));
		m_programDOFBlur->setUniform("uNearBlurRadiusPixels", (int)round(maximumCoCRadius));
		m_programDOFBlur->setUniform("uInvNearBlurRadiusPixels", 1.0f / maximumCoCRadius);
		m_programDOFBlur->setUniform("horizontal", true);
		m_programDOFBlur->setUniform("nearTexture", 0);
		m_programDOFBlur->setUniform("blurTexture", 1);

		m_programDOFBlur->use();
		m_vaoQuad->bind();
		m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
		m_vaoQuad->unbind();
		m_programDOFBlur->release();

		m_colorTexture->unbindActive(1);
		m_colorTexture->unbindActive(0);
		m_dofBlurFramebuffer->unbind();

		m_dofFramebuffer->bind();

		m_sphereNormalTexture->bindActive(0);
		m_surfaceNormalTexture->bindActive(1);
		m_programDOFBlur->setUniform("horizontal", false);
		m_programDOFBlur->setUniform("nearTexture", 0);
		m_programDOFBlur->setUniform("blurTexture", 1);

		m_programDOFBlur->use();		
		m_vaoQuad->bind();
		m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
		m_vaoQuad->unbind();
		m_programDOFBlur->release();

		m_surfaceNormalTexture->unbindActive(1);
		m_sphereNormalTexture->unbindActive(0);

		m_dofFramebuffer->unbind();

		m_shadeFramebuffer->bind();

		m_colorTexture->bindActive(0);
		m_sphereDiffuseTexture->bindActive(1);
		m_surfaceDiffuseTexture->bindActive(2);

		m_programDOFBlend->setUniform("maximumCoCRadius", maximumCoCRadius);
		m_programDOFBlend->setUniform("aparture", aparture);
		m_programDOFBlend->setUniform("focalDistance", focalDistance);
		m_programDOFBlend->setUniform("focalLength", focalLength);

		m_programDOFBlend->setUniform("colorTexture", 0);
		m_programDOFBlend->setUniform("nearTexture", 1);
		m_programDOFBlend->setUniform("blurTexture", 2);

		m_programDOFBlend->use();
		m_vaoQuad->bind();
		m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
		m_vaoQuad->unbind();
		m_programDOFBlend->release();

		m_sphereDiffuseTexture->bindActive(1);
		m_sphereDiffuseTexture->bindActive(0);
		m_shadeFramebuffer->unbind();
	}
	
	m_shadeFramebuffer->blit(GL_COLOR_ATTACHMENT0, {0,0,viewer()->viewportSize().x, viewer()->viewportSize().y}, Framebuffer::defaultFBO().get(), GL_BACK, { 0,0,viewer()->viewportSize().x, viewer()->viewportSize().y }, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

#ifdef BENCHMARK
	m_shadeQuery->end(GL_TIME_ELAPSED);
#endif

#ifdef STATISTICS
	Statistics *s = (Statistics*) m_statisticsBuffer->map();	
	std::string intersectionCount = std::to_string(s->intersectionCount);
	std::stringstream ss;
	ss << std::fixed << std::setprecision(2) << (float(s->intersectionCount) * 28.0f) / (1024.0f * 1024.0f);
	std::string intersectionMemory = ss.str();
	std::string totalPixelCount = std::to_string(s->totalPixelCount);
	std::string totalEntryCount = std::to_string(s->totalEntryCount);
	std::string maximumEntryCount = std::to_string(s->maximumEntryCount);
	std::string coverage = std::to_string(100.0*double(s->totalPixelCount) / double(viewportSize.x*viewportSize.y));

	*s = Statistics();

	m_statisticsBuffer->unmap();

	ImGui::Begin("Statistics");

	ImGui::InputText("Intersection Count", (char*)intersectionCount.c_str(),intersectionCount.size(), ImGuiInputTextFlags_ReadOnly);
	ImGui::InputText("Intersection Memory (MB)", (char*)intersectionMemory.c_str(), intersectionMemory.size(), ImGuiInputTextFlags_ReadOnly);
	ImGui::InputText("Total Pixel Count", (char*)totalPixelCount.c_str(), totalPixelCount.size(), ImGuiInputTextFlags_ReadOnly);
	ImGui::InputText("Total Enrty Count", (char*)totalEntryCount.c_str(), totalEntryCount.size(), ImGuiInputTextFlags_ReadOnly);
	ImGui::InputText("Maximum Entry Count", (char*)maximumEntryCount.c_str(), maximumEntryCount.size(), ImGuiInputTextFlags_ReadOnly);
	ImGui::InputText("Coverage", (char*)coverage.c_str(), coverage.size(), ImGuiInputTextFlags_ReadOnly);
	ImGui::End();
#endif	

#ifdef BENCHMARK
	m_sphereQuery->wait();
	m_spawnQuery->wait();
	m_surfaceQuery->wait();
	m_shadeQuery->wait();

	uint sphereElapsed = m_sphereQuery->get(GL_QUERY_RESULT);
	uint spawnElapsed = m_spawnQuery->get(GL_QUERY_RESULT);
	uint surfaceElapsed = m_surfaceQuery->get(GL_QUERY_RESULT);
	uint shadeElapsed = m_shadeQuery->get(GL_QUERY_RESULT);

	double sphereTime = double(sphereElapsed) / 1000000000.0;
	double spawnTime = double(spawnElapsed) / 1000000000.0;
	double surfaceTime = double(surfaceElapsed) / 1000000000.0;
	double shadeTime = double(shadeElapsed) / 1000000000.0;

	static uint counter = 0;

	static double sphereMin = std::numeric_limits<double>::max();
	static double spawnMin = std::numeric_limits<double>::max();
	static double surfaceMin = std::numeric_limits<double>::max();
	static double shadeMin = std::numeric_limits<double>::max();

	static double sphereMax = 0.0;
	static double spawnMax = 0.0;
	static double surfaceMax = 0.0;
	static double shadeMax = 0.0;

	static double sphereSum = 0.0;
	static double spawnSum = 0.0;
	static double surfaceSum = 0.0;
	static double shadeSum = 0.0;

	sphereMin = std::min(sphereTime, sphereMin);
	spawnMin = std::min(spawnTime, spawnMin);
	surfaceMin = std::min(surfaceTime, surfaceMin);
	shadeMin = std::min(shadeTime, shadeMin);

	sphereMax = std::max(sphereTime, sphereMax);
	spawnMax = std::max(spawnTime, spawnMax);
	surfaceMax = std::max(surfaceTime, surfaceMax);
	shadeMax = std::max(shadeTime, shadeMax);

	sphereSum += sphereTime;
	spawnSum += spawnTime;
	surfaceSum += surfaceTime;
	shadeSum += shadeTime;

	if (m_benchmarkCounter >= 3*360)
	{
		sphereSum /= double(m_benchmarkCounter);
		spawnSum /= double(m_benchmarkCounter);
		surfaceSum /= double(m_benchmarkCounter);
		shadeSum /= double(m_benchmarkCounter);
		
		double phaseSum = sphereSum + spawnSum + surfaceSum + shadeSum;
		double phaseMin = sphereMin + spawnMin + surfaceMin + shadeMin;
		double phaseMax = sphereMax + spawnMax + surfaceMax + shadeMax;

		/*
		pdbid,atomcount,
		sphereavgla,spheremaxla,sphereminla,
		spawnavgla,spawnmaxla,spawnminla,
		surfaceavgla,surfacemaxla,surfaceminla,
		shadeavgla,shademaxla,shademinla,
		fpsavgla,fpsminla,fpsmaxla,
		sphereavglb, spheremaxlb, sphereminlb,
		spawnavglb, spawnmaxlb, spawnminlb,
		surfaceavglb, surfacemaxlb, surfaceminlb,
		shadeavglb, shademaxlb, shademinlb,
		fpsavglb, fpsminlb, fpsmaxlb,
		sphereavglc, spheremaxlc, sphereminlc,
		spawnavglc, spawnmaxlc, spawnminlc,
		surfaceavglc, surfacemaxlc, surfaceminlc,
		shadeavglc, shademaxlc, shademinlc,
		fpsavglc, fpsminlc, fpsmaxlc,
		sphereavgld, spheremaxld, sphereminld,
		spawnavgld, spawnmaxld, spawnminld,
		surfaceavgld, surfacemaxld, surfaceminld,
		shadeavgld, shademaxld, shademinld,
		fpsavgld, fpsminld, fpsmaxld,
		sphereavgma, spheremaxma, sphereminma,
		spawnavgma, spawnmaxma, spawnminma,
		surfaceavgma, surfacemaxma, surfaceminma,
		shadeavgma, shademaxma, shademinma,
		fpsavgma, fpsminma, fpsmaxma,
		sphereavgmb, spheremaxmb, sphereminmb,
		spawnavgmb, spawnmaxmb, spawnminmb,
		surfaceavgmb, surfacemaxmb, surfaceminmb,
		shadeavgmb, shademaxmb, shademinmb,
		fpsavgmb, fpsminmb, fpsmaxmb,
		sphereavgmc, spheremaxmc, sphereminmc,
		spawnavgmc, spawnmaxmc, spawnminmc,
		surfaceavgmc, surfacemaxmc, surfaceminmc,
		shadeavgmc, shademaxmc, shademinmc,
		fpsavgmc, fpsminmc, fpsmaxmc,
		sphereavgmd, spheremaxmd, sphereminmd,
		spawnavgmd, spawnmaxmd, spawnminmd,
		surfaceavgmd, surfacemaxmd, surfaceminmd,
		shadeavgmd, shademaxmd, shademinmd,
		fpsavgmd, fpsminmd, fpsmaxmd,
		sphereavgha, spheremaxha, sphereminha,
		spawnavgha, spawnmaxha, spawnminha,
		surfaceavgha, surfacemaxha, surfaceminha,
		shadeavgha, shademaxha, shademinha,
		fpsavgha, fpsminha, fpsmaxha,
		sphereavghb, spheremaxhb, sphereminhb,
		spawnavghb, spawnmaxhb, spawnminhb,
		surfaceavghb, surfacemaxhb, surfaceminhb,
		shadeavghb, shademaxhb, shademinhb,
		fpsavghb, fpsminhb, fpsmaxhb,
		sphereavghc, spheremaxhc, sphereminhc,
		spawnavghc, spawnmaxhc, spawnminhc,
		surfaceavghc, surfacemaxhc, surfaceminhc,
		shadeavghc, shademaxhc, shademinhc,
		fpsavghc, fpsminhc, fpsmaxhc,
		sphereavghd, spheremaxhd, sphereminhd,
		spawnavghd, spawnmaxhd, spawnminhd,
		surfaceavghd, surfacemaxhd, surfaceminhd,
		shadeavghd, shademaxhd, shademinhd,
		fpsavghd, fpsminhd, fpsmaxhd,
		*/
		if (!m_benchmarkWarmup)
		{
			std::cout << "," << sphereSum * 100.0 << ",";
			std::cout << sphereMax * 100.0 << ",";
			std::cout << sphereMin * 100.0 << ",";
			
			std::cout << spawnSum * 100.0 << ",";
			std::cout << spawnMax * 100.0 << ",";
			std::cout << spawnMin * 100.0 << ",";

			std::cout << surfaceSum * 100.0 << ",";
			std::cout << surfaceMax * 100.0 << ",";
			std::cout << surfaceMin * 100.0 << ",";

			std::cout << shadeSum * 100.0 << ",";
			std::cout << shadeMax * 100.0 << ",";
			std::cout << shadeMin * 100.0 << ",";

			std::cout << std::round(1.0 / phaseSum) << ",";
			std::cout << std::round(1.0 / phaseMax) << ",";
			std::cout << std::round(1.0 / phaseMin);
		}

		sphereMin = std::numeric_limits<double>::max();
		spawnMin = std::numeric_limits<double>::max();
		surfaceMin = std::numeric_limits<double>::max();
		shadeMin = std::numeric_limits<double>::max();

		sphereMax = 0.0;
		spawnMax = 0.0;
		surfaceMax = 0.0;
		shadeMax = 0.0;

		sphereSum = 0.0;
		spawnSum = 0.0;
		surfaceSum = 0.0;
		shadeSum = 0.0;

		m_benchmarkCounter = 0;

		if (!m_benchmarkWarmup)
			m_benchmarkIndex++;

		m_benchmarkWarmup = false;

		if (m_benchmarkIndex >= sharpnessValues.size())
		{
			m_benchmarkPhase++;
			m_benchmarkIndex = 0;
			m_benchmarkWarmup = true;
		}
	}

#endif

	currentState->apply();
}