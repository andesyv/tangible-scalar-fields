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

	m_intersectionBuffer->setStorage(sizeof(vec3) * 1024*1024*128 + sizeof(uint), nullptr, gl::GL_NONE_BIT);
	m_statisticsBuffer->setStorage(sizeof(s), (void*)&s, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);

	// shader storage buffer object for current depth entries
	m_depthRangeBuffer->setStorage(sizeof(uint) * 2, nullptr, gl::GL_NONE_BIT);
	m_geomMeanBuffer->setStorage(sizeof(int), nullptr, gl::GL_NONE_BIT);

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
	m_fragmentShaderSourceDensityEstimation = Shader::sourceFromFile("./res/sphere/density-fs.glsl");
	m_vertexShaderSourceGeomMean = Shader::sourceFromFile("./res/sphere/geomMean-vs.glsl");
	m_fragmentShaderSourceGeomMean = Shader::sourceFromFile("./res/sphere/geomMean-fs.glsl");
	
	
	m_vertexShaderTemplateSphere = Shader::applyGlobalReplacements(m_vertexShaderSourceSphere.get());
	m_geometryShaderTemplateSphere = Shader::applyGlobalReplacements(m_geometryShaderSourceSphere.get());
	m_fragmentShaderTemplateSphere = Shader::applyGlobalReplacements(m_fragmentShaderSourceSphere.get());
	m_fragmentShaderTemplateSpawn = Shader::applyGlobalReplacements(m_fragmentShaderSourceSpawn.get());
	m_vertexShaderTemplateImage = Shader::applyGlobalReplacements(m_vertexShaderSourceImage.get());
	m_geometryShaderTemplateImage = Shader::applyGlobalReplacements(m_geometryShaderSourceImage.get());
	m_fragmentShaderTemplateSurface = Shader::applyGlobalReplacements(m_fragmentShaderSourceSurface.get());
	m_fragmentShaderTemplateDensityEstimation = Shader::applyGlobalReplacements(m_fragmentShaderSourceDensityEstimation.get());
	m_vertexShaderTemplateGeomMean = Shader::applyGlobalReplacements(m_vertexShaderSourceGeomMean.get());
	m_fragmentShaderTemplateGeomMean = Shader::applyGlobalReplacements(m_fragmentShaderSourceGeomMean.get());
	m_fragmentShaderTemplateAOSample = Shader::applyGlobalReplacements(m_fragmentShaderSourceAOSample.get());
	m_fragmentShaderTemplateAOBlur = Shader::applyGlobalReplacements(m_fragmentShaderSourceAOBlur.get());
	m_fragmentShaderTemplateShade = Shader::applyGlobalReplacements(m_fragmentShaderSourceShade.get());

	m_vertexShaderSphere = Shader::create(GL_VERTEX_SHADER, m_vertexShaderTemplateSphere.get());
	m_geometryShaderSphere = Shader::create(GL_GEOMETRY_SHADER, m_geometryShaderTemplateSphere.get());
	m_fragmentShaderSphere = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateSphere.get());
	m_fragmentShaderSpawn = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateSpawn.get());
	m_vertexShaderImage = Shader::create(GL_VERTEX_SHADER, m_vertexShaderTemplateImage.get());
	m_geometryShaderImage = Shader::create(GL_GEOMETRY_SHADER, m_geometryShaderTemplateImage.get());
	m_fragmentShaderSurface = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateSurface.get());
	m_fragmentShaderDensityEstimation = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateDensityEstimation.get());
	m_fragmentShaderGeomMean = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateGeomMean.get());
	m_vertexShaderGeomMean = Shader::create(GL_VERTEX_SHADER, m_vertexShaderTemplateGeomMean.get());
	m_fragmentShaderAOSample = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateAOSample.get());
	m_fragmentShaderAOBlur = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateAOBlur.get());
	m_fragmentShaderShade = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateShade.get());

	m_programSphere->attach(m_vertexShaderSphere.get(), m_geometryShaderSphere.get(), m_fragmentShaderSphere.get());
	m_programDensityEstimation->attach(m_vertexShaderSphere.get(), m_geometryShaderSphere.get(), m_fragmentShaderDensityEstimation.get());
	m_programSpawn->attach(m_vertexShaderSphere.get(), m_geometryShaderSphere.get(), m_fragmentShaderSpawn.get());
	m_programSurface->attach(m_vertexShaderImage.get(), m_geometryShaderImage.get(), m_fragmentShaderSurface.get());
	m_programAOSample->attach(m_vertexShaderImage.get(), m_geometryShaderImage.get(), m_fragmentShaderAOSample.get());
	m_programAOBlur->attach(m_vertexShaderImage.get(), m_geometryShaderImage.get(), m_fragmentShaderAOBlur.get());
	m_programShade->attach(m_vertexShaderImage.get(), m_geometryShaderImage.get(), m_fragmentShaderShade.get());
	m_programGeomMean->attach(m_vertexShaderGeomMean.get(), m_fragmentShaderGeomMean.get());

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

	m_kernelDensityTexture = Texture::create(GL_TEXTURE_2D);
	m_kernelDensityTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_kernelDensityTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_kernelDensityTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_kernelDensityTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_kernelDensityTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_pilotKernelDensityTexture = Texture::create(GL_TEXTURE_2D);
	m_pilotKernelDensityTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_pilotKernelDensityTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_pilotKernelDensityTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);		// GL_TEXTURE_BORDER_COLOR is (0,0,0,0) by default
	m_pilotKernelDensityTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);		// GL_TEXTURE_BORDER_COLOR is (0,0,0,0) by default
	m_pilotKernelDensityTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_scatterPlotTexture = Texture::create(GL_TEXTURE_2D);
	m_scatterPlotTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_scatterPlotTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_scatterPlotTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_scatterPlotTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_scatterPlotTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_colorMapTexture = Texture::create(GL_TEXTURE_1D);
	m_colorMapTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_colorMapTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_colorMapTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_colorMapTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

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
	m_sphereFramebuffer->attachTexture(GL_COLOR_ATTACHMENT2, m_kernelDensityTexture.get());
	m_sphereFramebuffer->attachTexture(GL_COLOR_ATTACHMENT3, m_scatterPlotTexture.get());
	m_sphereFramebuffer->attachTexture(GL_COLOR_ATTACHMENT4, m_pilotKernelDensityTexture.get());
	m_sphereFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
	m_sphereFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 });

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
		m_fragmentShaderSourceDensityEstimation.get(),
		m_vertexShaderSourceGeomMean.get(),
		m_fragmentShaderSourceGeomMean.get(),
		m_fragmentShaderSourceSpawn.get(),
		m_fragmentShaderSourceSurface.get(),
		m_fragmentShaderSourceAOSample.get(),
		m_fragmentShaderSourceAOBlur.get(),
		m_fragmentShaderSourceShade.get()
		});
}

void SphereRenderer::display()
{
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
		m_kernelDensityTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		m_pilotKernelDensityTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		m_scatterPlotTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	}

	// retrieve/compute all necessary matrices and related properties
	const mat4 viewMatrix = viewer()->viewTransform();
	const mat4 inverseViewMatrix = inverse(viewMatrix);
	const mat4 modelViewMatrix = viewer()->modelViewTransform();
	const mat4 inverseModelViewMatrix = inverse(modelViewMatrix);
	const mat4 modelLightMatrix = viewer()->modelLightTransform();
	const mat4 inverseModelLightMatrix = inverse(modelLightMatrix);
	const mat4 modelViewProjectionMatrix = viewer()->modelViewProjectionTransform();
	const mat4 inverseModelViewProjectionMatrix = inverse(modelViewProjectionMatrix);
	const mat4 projectionMatrix = viewer()->projectionTransform();
	const mat4 inverseProjectionMatrix = inverse(projectionMatrix);
	const mat3 normalMatrix = mat3(transpose(inverseModelViewMatrix));
	const mat3 inverseNormalMatrix = inverse(normalMatrix);
	const ivec2 viewportSize = viewer()->viewportSize();

	vec4 worldCameraPosition = inverseModelViewMatrix * vec4(0.0f, 0.0f, 0.0f, 1.0f);
	vec4 worldLightPosition = inverseModelLightMatrix * vec4(0.0f, 0.0f, 0.0f, 1.0f);
	
	double mouseX, mouseY;
	glfwGetCursorPos(viewer()->window(), &mouseX, &mouseY);
	vec2 focusPosition = vec2(2.0f*float(mouseX) / float(viewportSize.x) - 1.0f, -2.0f*float(mouseY) / float(viewportSize.y) + 1.0f);

	vec4 projectionInfo(float(-2.0 / (viewportSize.x * projectionMatrix[0][0])),
		float(-2.0 / (viewportSize.y * projectionMatrix[1][1])),
		float((1.0 - (double)projectionMatrix[0][2]) / projectionMatrix[0][0]),
		float((1.0 + (double)projectionMatrix[1][2]) / projectionMatrix[1][1]));

	float projectionScale = float(viewportSize.y) / fabs(2.0f / projectionMatrix[1][1]);

	// adapted version of "pearl" from teapots.c File Reference
	// Mark J. Kilgard, Silicon Graphics, Inc. 1994 - http://web.archive.org/web/20100725103839/http://www.cs.utk.edu/~kuck/materials_ogl.htm
	static vec3 ambientMaterial(0.20725f, 0.20725f, 0.20725f);
	static vec3 diffuseMaterial(1, 1, 1);// 0.829f, 0.829f, 0.829f);
	static vec3 specularMaterial(0.296648f, 0.296648f, 0.296648f);
	static float shininess = 11.264f;

	vec4 nearPlane = inverseProjectionMatrix * vec4(0.0, 0.0, -1.0, 1.0);
	nearPlane /= nearPlane.w;

	// boolean variable used to automatically update the data
	static bool dataChanged = false;
	//static bool adjust = true;

	// Scatterplot GUI --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

	ImGui::Begin("Illuminated Point Plots");

	if (ImGui::CollapsingHeader("CSV-Files"), ImGuiTreeNodeFlags_DefaultOpen)
	{

		ImGui::Combo("Files", &m_fileDataID, m_guiFileNames.c_str());

		if (m_fileDataID != m_oldFileDataID)
		{
			//std::cout << "File selection event - " << "File: " << m_fileDataID << "\n";

			// reset column names
			m_guiColumnNames = { 'N', 'o', 'n', 'e', '\0' };

			if (m_fileDataID != 0) {
			
				// initialize table
				viewer()->scene()->table()->load("./dat/" + m_fileNames[m_fileDataID]);

				//-------------------------------------------------------------------------------------------------------------------------------------------
				/*std::string location = "C:/Users/thoma/OneDrive/Desktop/PhD Bergen/Papers/Paper 1/Matlab/UserStudyPrep/Output/" + m_fileNames[m_fileDataID].substr(0, m_fileNames[m_fileDataID].length() - 4) + "/" + m_fileNames[m_fileDataID].substr(0, m_fileNames[m_fileDataID].length() - 4) + "_BoundingBox.csv";
				std::ifstream myfile1(location);
				if (myfile1.is_open())
				{
					std::string number;
					std::vector <std::string> record;

					// skip header
					std::getline(myfile1, number);

					while (std::getline(myfile1, number, ','))
					{
						record.push_back(number);
					}
					myfile1.close();

					m_minx = stoi(record.at(0));
					m_maxx = stoi(record.at(1));
					m_miny = stoi(record.at(2));
					m_maxy = stoi(record.at(3));
				}

				location = "C:/Users/thoma/OneDrive/Desktop/PhD Bergen/Papers/Paper 1/Matlab/UserStudyPrep/Output/" + m_fileNames[m_fileDataID].substr(0, m_fileNames[m_fileDataID].length() - 4) + "/" + m_fileNames[m_fileDataID].substr(0, m_fileNames[m_fileDataID].length() - 4) + "_RenderParameters.csv";
				std::ifstream myfile2(location);
				if (myfile2.is_open())
				{
					std::string number;
					std::vector <std::string> record;

					// skip header
					std::getline(myfile2, number);

					while (std::getline(myfile2, number, ','))
					{
						record.push_back(number);
					}
					myfile2.close();

					// parse values form file
					m_radiusMultiplier = stof(record.at(0));
					m_gaussScale = stof(record.at(1));
					m_sigma = stof(record.at(2));
					m_alpha = stof(record.at(3));
					
					if (record.at(4) == "No")
						m_adaptiveKDE = false;
					else if(record.at(4) == "Yes")
						m_adaptiveKDE = true;
					
					//viewer()->setBackgroundColor(glm::vec3());
					//viewer()->setSamplePointColor();
					//ambientMaterial = glm::vec3();
					//diffuseMaterial = glm::vec3();
					//specularMaterial = glm::vec3();

					shininess = stof(record.at(11));

					m_occlusionIntensity = stof(record.at(12));
					m_occlusionRadius = stof(record.at(13));
					
					//m_heatMapGUI // is Plasma by default

					m_scatterScale = stof(record.at(15));
					m_opacityScale = stof(record.at(16));
				}

				adjust = true;*/
				//-------------------------------------------------------------------------------------------------------------------------------------------

				// extract column names and prepare GUI
				std::vector<std::string> tempNames = viewer()->scene()->table()->getColumnNames();

				for (std::vector<std::string>::iterator it = tempNames.begin(); it != tempNames.end(); ++it) {
					m_guiColumnNames += *it + '\0';
				}

				// provide default selections assuming
				m_xAxisDataID = 1;		// contains the X-values
				m_yAxisDataID = 2;		// contains the Y-values
				m_radiusDataID = 3;		// contains the radii
				m_colorDataID = 4;		// contains the colors
			}

			// update status
			m_oldFileDataID = m_fileDataID;
			dataChanged = true;
		}

		// show all column names from selected CSV file
		ImGui::Text("Selected Columns:");
		ImGui::Combo("X-axis", &m_xAxisDataID, m_guiColumnNames.c_str());
		ImGui::Combo("Y-axis", &m_yAxisDataID, m_guiColumnNames.c_str());
		ImGui::Combo("Radius", &m_radiusDataID, m_guiColumnNames.c_str());
		ImGui::Combo("Color", &m_colorDataID, m_guiColumnNames.c_str());

		if (ImGui::Button("Update") || dataChanged)
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
			vertexBinding->setAttribute(0);
			vertexBinding->setBuffer(m_xColumnBuffer.get(), 0, sizeof(float));
			vertexBinding->setFormat(1, GL_FLOAT);
			m_vao->enable(0);

			vertexBinding = m_vao->binding(1);
			vertexBinding->setAttribute(1);
			vertexBinding->setBuffer(m_yColumnBuffer.get(), 0, sizeof(float));
			vertexBinding->setFormat(1, GL_FLOAT);
			m_vao->enable(1);

			vertexBinding = m_vao->binding(2);
			vertexBinding->setAttribute(2);
			vertexBinding->setBuffer(m_radiusColumnBuffer.get(), 0, sizeof(float));
			vertexBinding->setFormat(1, GL_FLOAT);
			m_vao->enable(2);

			vertexBinding = m_vao->binding(3);
			vertexBinding->setAttribute(3);
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
			
			// store diameter of current scatter plot and initialize light position
			viewer()->m_scatterPlotDiameter = sqrt(pow(boundingBoxSize.x, 2) + pow(boundingBoxSize.y, 2));

			// initial position of the light source (azimuth 120 degrees, elevation 45 degrees, 5 times the distance to the object in center) ---------------------------------------------------------------------------------------------------------
			glm::mat4 viewTransform = viewer()->viewTransform();
			glm::vec3 initLightDir = normalize(glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(120.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
			glm::mat4 newLightTransform = glm::inverse(viewTransform)*glm::translate(mat4(1.0f), (-5*viewer()->m_scatterPlotDiameter*initLightDir))*viewTransform;
			viewer()->setLightTransform(newLightTransform);
			//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
		}

		// update status
		dataChanged = false;
	}

	if (ImGui::CollapsingHeader("Kernel Density Estimation"))
	{
		ImGui::SliderFloat("Radius-Scale", &m_radiusMultiplier, 0.001f, 256.0f);
		ImGui::SliderFloat("Gauss-Scale", &m_gaussScale, 0.01f, 1.0f);
		ImGui::SliderFloat("Sigma", &m_sigma, 1.0f, 500.0f);
		ImGui::SliderFloat("Alpha", &m_alpha, 0.01f, 1.0f);

		// section for adaptive kernels
		ImGui::Checkbox("Zoom-Dependent Kernel", &m_adaptKernel);
		ImGui::Checkbox("Adaptive Density Estimation", &m_adaptiveKDE);
	}

	if (ImGui::CollapsingHeader("Lighting"), ImGuiTreeNodeFlags_DefaultOpen)
	{
		ImGui::ColorEdit3("Ambient", (float*)&ambientMaterial);
		ImGui::ColorEdit3("Diffuse", (float*)&diffuseMaterial);
		ImGui::ColorEdit3("Specular", (float*)&specularMaterial);
		ImGui::SliderFloat("Shininess", &shininess, 1.0f, 256.0f);
		ImGui::SliderFloat("SSAO Intensity", &m_occlusionIntensity, 0.01f, 128.0f);
		ImGui::SliderFloat("SSAO Radius", &m_occlusionRadius, 0.01f, 1.0f);
		ImGui::Checkbox("Surface Illumination", &m_surfaceIllumination);
		ImGui::Checkbox("Ambient Occlusion Enabled", &m_ambientOcclusion);
	}

	if (ImGui::CollapsingHeader("Color Maps"), ImGuiTreeNodeFlags_DefaultOpen)
	{
		// show all available color-maps
		ImGui::Combo("Maps", &m_colorMap, "None\0Bone\0Cubehelix\0GistEart\0GnuPlot2\0Grey\0Inferno\0Magma\0Plasma\0PuBuGn\0Rainbow\0Summer\0Virdis\0Winter\0Wista\0YlGnBu\0YlOrRd\0");

		// allow the user to load a discrete version of the color map
		ImGui::Checkbox("GUI Heatmap", &m_heatMapGUI);
		ImGui::Checkbox("Discrete Colors (7)", &m_discreteMap);

		// load new texture if either the texture has changed or the type has changed from discrete to continuous or vice versa
		if (m_colorMap != m_oldColorMap || m_discreteMap != m_oldDiscreteMap)
		{
			if (m_colorMap > 0) {
				std::vector<std::string> colorMapFilenames = { "./dat/colormaps/bone_1D.png", "./dat/colormaps/cubehelix_1D.png", "./dat/colormaps/gist_earth_1D.png",  "./dat/colormaps/gnuplot2_1D.png" , 
					"./dat/colormaps/grey_1D.png", "./dat/colormaps/inferno_1D.png", "./dat/colormaps/magma_1D.png", "./dat/colormaps/plasma_1D.png", "./dat/colormaps/PuBuGn_1D.png", 
					"./dat/colormaps/rainbow_1D.png", "./dat/colormaps/summer_1D.png", "./dat/colormaps/virdis_1D.png", "./dat/colormaps/winter_1D.png", "./dat/colormaps/wista_1D.png", "./dat/colormaps/YlGnBu_1D.png", 
					"./dat/colormaps/YlOrRd_1D.png" };

				uint colorMapWidth, colorMapHeight;
				std::vector<unsigned char> colorMapImage;

				std::string textureName = colorMapFilenames[m_colorMap - 1];

				if (m_discreteMap)
				{
					std::string fileExtension = "_discrete7";

					// insert the discrete identifier "_discrete7" before the file extension ".png"
					textureName.insert(textureName.length() - 4, fileExtension);
				}

				uint error = lodepng::decode(colorMapImage, colorMapWidth, colorMapHeight, textureName);

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

			// update status
			m_oldColorMap = m_colorMap;
			m_oldDiscreteMap = m_discreteMap;
		}
	}

	if (ImGui::CollapsingHeader("Blending Function"), ImGuiTreeNodeFlags_DefaultOpen)
	{
		ImGui::Combo("Blending", &m_blendingFunction, "None\0CurvatureBased\0DistanceBased\0NormalBased\0ScatterPlot\0");
		ImGui::Combo("Color Scheme", &m_colorScheme, "None\0Scheme1\0Scheme2\0");
		
		ImGui::SliderFloat("Scatter Plot Scale", &m_scatterScale, 0.001f, 10.0f);
		ImGui::SliderFloat("Opacity Scale", &m_opacityScale, 0.001f, 1.0f);
		ImGui::Checkbox("Invert Function", &m_invertFunction);
	}

	if (ImGui::CollapsingHeader("Contour Lines"))
	{
		ImGui::Checkbox("Iso-Contours", &m_countourLines);
		ImGui::SliderInt("Number of Contours", &m_contourLinesCount, 1, 25);
		ImGui::SliderFloat("Contour Thickness", &m_contourThickness, 0.001f, 0.1f);
	}

	if (ImGui::CollapsingHeader("Magic Lens"))
	{
		ImGui::Checkbox("Enable Lens", &m_lenseEnabled);
		ImGui::SliderFloat("Lens Size", &m_lensSize, 0.1f, 0.5f);
		ImGui::SliderFloat("Lens Sigma", &viewer()->m_scrollWheelSigma, 0.1f, 1.0f);
	}

	ImGui::End();

	// ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

	// do not render if either the dataset was not loaded or the window is minimized 
	if (viewer()->scene()->table()->activeTableData().size() == 0 || viewer()->viewportSize().x == 0 || viewer()->viewportSize().y == 0)
	{
		return;
	}

	int vertexCount = int(viewer()->scene()->table()->activeTableData()[0].size());

	std::string defines = "";

	if (m_lenseEnabled)
		defines += "#define LENSING\n";

	if (m_ambientOcclusion)
		defines += "#define AMBIENT\n";

	if (m_colorMapLoaded)
		defines += "#define COLORMAP\n";

	if(m_surfaceIllumination)
		defines += "#define ILLUMINATION\n";

	if (m_colorScheme == 1)
		defines += "#define COLORSCHEME1\n";
	else if (m_colorScheme == 2)
		defines += "#define COLORSCHEME2\n";

	if (m_blendingFunction == 1)
		defines += "#define CURVEBLENDING\n";
	else if (m_blendingFunction == 2)
		defines +="#define DISTANCEBLENDING\n";
	else if (m_blendingFunction == 3)
		defines += "#define NORMALBLENDING\n";
	else if (m_blendingFunction == 4)
		defines += "#define SCATTERPLOT\n";

	if (m_invertFunction)
		defines += "#define INVERTFUNCTION\n";

	if (m_adaptKernel)
		defines += "#define ADAPTIVEKERNEL\n";

	if(m_adaptiveKDE)
		defines += "#define ADAPTIVEKDE\n";

	if (m_countourLines)
		defines += "#define CONTOURLINES\n";

	if(m_colorMapLoaded && m_heatMapGUI)
		defines += "#define HEATMAP_GUI\n";

	if (defines != m_shaderSourceDefines->string())
	{
		m_shaderSourceDefines->setString(defines);

		for (auto& s : shaderFiles())
			s->reload();
	}

	m_sphereFramebuffer->bind();
	glClearDepth(1.0f);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);

	// make sure spheres are drawn on top of each other
	glDepthFunc(GL_ALWAYS);

	// test different blending options interactively: --------------------------------------------------
	// https://andersriggelsen.dk/glblendfunc.php

	// allow blending for the classical scatter plot color-attachment (3) of the sphere frame-buffer
	glEnablei(GL_BLEND, 3);				
	glBlendFunci(3, GL_ONE, GL_ONE);
	glBlendEquationi(3, GL_FUNC_ADD);
	// -------------------------------------------------------------------------------------------------

	m_programSphere->setUniform("modelViewMatrix", modelViewMatrix);
	m_programSphere->setUniform("projectionMatrix", projectionMatrix);
	m_programSphere->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);
	m_programSphere->setUniform("inverseModelViewProjectionMatrix", inverseModelViewProjectionMatrix);

	// GUI dependent color selection
	m_programSphere->setUniform("samplePointColor", viewer()->samplePointColor());

	// point radius is dependent on GUI variable "Scatter Plot Scale"
	m_programSphere->setUniform("radiusScale", m_scatterScale);
	m_programSphere->setUniform("clipRadiusScale", m_scatterScale);
	m_programSphere->setUniform("nearPlaneZ", nearPlane.z);

	m_vao->bind();

	m_programSphere->use();
	m_vao->drawArrays(GL_POINTS, 0, vertexCount);
	m_programSphere->release();
	
	m_vao->unbind();

	// disable blending for draw buffer 3 (classical scatter plot)
	glDisablei(GL_BLEND, 3);

	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	glDepthFunc(GL_ALWAYS);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_FALSE);		

	if (m_adaptiveKDE)
	{
		// allow writing to the pilot density estimation texture/ draw buffer (4) of the sphere frame-buffer
		glColorMaski(4, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		// test different blending options interactively: ------------------------------------
		// https://andersriggelsen.dk/glblendfunc.php

		// allow blending for the pilot KDE estimation color-attachment (4)
		glEnablei(GL_BLEND, 4);
		glBlendFunci(4, GL_ONE, GL_ONE);
		glBlendEquationi(4, GL_FUNC_ADD);
		// -----------------------------------------------------------------------------------

		m_programDensityEstimation->setUniform("modelViewMatrix", modelViewMatrix);
		m_programDensityEstimation->setUniform("projectionMatrix", projectionMatrix);
		m_programDensityEstimation->setUniform("inverseModelViewProjectionMatrix", inverseModelViewProjectionMatrix);

		// sphere of influence radius is dependent on GUI variable "Radius-Scale"
		m_programDensityEstimation->setUniform("radiusScale", m_radiusMultiplier);
		m_programDensityEstimation->setUniform("clipRadiusScale", m_radiusMultiplier);
		m_programDensityEstimation->setUniform("nearPlaneZ", nearPlane.z);

		// KDE parameters
		m_programDensityEstimation->setUniform("sigma2", m_sigma);
		m_programDensityEstimation->setUniform("gaussScale", m_gaussScale);

		m_vao->bind();

		m_programDensityEstimation->use();
		m_vao->drawArrays(GL_POINTS, 0, vertexCount);
		m_programDensityEstimation->release();

		m_vao->unbind();

		// disable blending for pilot density estimation (4)
		glDisablei(GL_BLEND, 4);


		// Additional render pass to estimate the geometric mean -----------------------------
		m_geomMeanBuffer->bindBase(GL_SHADER_STORAGE_BUFFER, 1);

		const int clearValue = 0;
		m_geomMeanBuffer->clearSubData(GL_R32UI, 0, sizeof(int), GL_RED_INTEGER, GL_UNSIGNED_INT, &clearValue);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);

		// bind texture containing density from initial pilot density estimation
		m_pilotKernelDensityTexture->bindActive(0);

		// pilot estimation
		m_programGeomMean->setUniform("pilotDensityTexture", 0);
		m_programGeomMean->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);

		m_vao->bind();

		m_programGeomMean->use();
		m_vao->drawArrays(GL_POINTS, 0, vertexCount);
		m_programGeomMean->release();

		m_vao->unbind();
		// -----------------------------------------------------------------------------------

	}

	// allow writing to the kernel density texture/ draw buffer (2) of the sphere frame-buffer
	glColorMaski(2, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	// test different blending options interactively: ------------------------------------
	// https://andersriggelsen.dk/glblendfunc.php

	// allow blending for the KDE color-attachment (2)
	glEnablei(GL_BLEND, 2);				
	glBlendFunci(2, GL_ONE, GL_ONE);	
	glBlendEquationi(2, GL_FUNC_ADD);		
	// -----------------------------------------------------------------------------------

	if (m_adaptiveKDE)
	{
		// pilot estimation
		m_programSpawn->setUniform("pilotDensityTexture", 0);

		// window properties
		m_programSpawn->setUniform("windowWidth", viewer()->m_windowWidth);
		m_programSpawn->setUniform("windowHeight", viewer()->m_windowHeight);

		//pass number of sample points
		m_programSpawn->setUniform("samplesCount", (float)vertexCount);
		m_programSpawn->setUniform("alpha", m_alpha);
	}

	m_programSpawn->setUniform("modelViewMatrix", modelViewMatrix);
	m_programSpawn->setUniform("projectionMatrix", projectionMatrix);
	m_programSpawn->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);
	m_programSpawn->setUniform("inverseModelViewProjectionMatrix", inverseModelViewProjectionMatrix);

	// sphere of influence radius is dependent on GUI variable "Radius-Scale"
	m_programSpawn->setUniform("radiusScale", m_radiusMultiplier);
	m_programSpawn->setUniform("clipRadiusScale", m_radiusMultiplier);
	m_programSpawn->setUniform("nearPlaneZ", nearPlane.z);

	// KDE parameters
	m_programSpawn->setUniform("sigma2", m_sigma);
	m_programSpawn->setUniform("gaussScale", m_gaussScale);

	// Focus and Context (mouse position for lens)
	m_programSpawn->setUniform("focusPosition", focusPosition);
	m_programSpawn->setUniform("lensSize", m_lensSize);
	m_programSpawn->setUniform("lensSigma", viewer()->m_scrollWheelSigma);

	// pass aspect ratio to the shader to make sure lens is always a circle and does not degenerate to an ellipse
	m_programSpawn->setUniform("aspectRatio", viewer()->m_windowHeight / viewer()->m_windowWidth);
	
	m_vao->bind();

	m_programSpawn->use();
	m_vao->drawArrays(GL_POINTS, 0, vertexCount);
	m_programSpawn->release();

	m_vao->unbind();
		
	// disable blending for (adaptive) KDE draw buffer (2)
	glDisablei(GL_BLEND, 2);		

	//m_intersectionBuffer->unbind(GL_SHADER_STORAGE_BUFFER);

	if (m_adaptiveKDE)
	{
		m_pilotKernelDensityTexture->unbindActive(0);
		m_geomMeanBuffer->unbind(GL_SHADER_STORAGE_BUFFER);
	}

	m_sphereFramebuffer->unbind();

	glMemoryBarrier(GL_ALL_BARRIER_BITS);// GL_TEXTURE_FETCH_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	m_surfaceFramebuffer->bind();
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);

	glClearDepth(1.0f);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_kernelDensityTexture->bindActive(7);
	m_scatterPlotTexture->bindActive(8);

	// SSBO
	//m_intersectionBuffer->bindBase(GL_SHADER_STORAGE_BUFFER, 1);
	//m_statisticsBuffer->bindBase(GL_SHADER_STORAGE_BUFFER, 2);
	m_depthRangeBuffer->bindBase(GL_SHADER_STORAGE_BUFFER, 3);

	// SSBO --------------------------------------------------------------------------------------------------------------------------------------------------
	const uint minClearValue = UINT_MAX;
	const uint maxClearValue = 0;

	// min/ max depth range
	m_depthRangeBuffer->clearSubData(GL_R32UI, 0, sizeof(uint), GL_RED_INTEGER, GL_UNSIGNED_INT, &minClearValue);
	m_depthRangeBuffer->clearSubData(GL_R32UI, 1 * sizeof(uint), sizeof(uint), GL_RED_INTEGER, GL_UNSIGNED_INT, &maxClearValue);
	// -------------------------------------------------------------------------------------------------------------------------------------------------------

	m_programSurface->setUniform("modelViewMatrix", modelViewMatrix);
	m_programSurface->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);
	m_programSurface->setUniform("inverseModelViewProjectionMatrix", inverseModelViewProjectionMatrix);
	
	m_programSurface->setUniform("kernelDensityTexture", 7);
	m_programSurface->setUniform("scatterPlotTexture", 8);

	m_vaoQuad->bind();

	m_programSurface->use();
	m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
	m_programSurface->release();

	m_vaoQuad->unbind();

	//m_depthRangeBuffer->unbind(GL_SHADER_STORAGE_BUFFER);

	m_scatterPlotTexture->unbindActive(8);
	m_kernelDensityTexture->unbindActive(7);

	m_surfaceFramebuffer->unbind();

	if (m_ambientOcclusion)
	{
		//glMemoryBarrier(GL_ALL_BARRIER_BITS);
		m_aoFramebuffer->bind();

		m_programAOSample->setUniform("projectionInfo", projectionInfo);
		m_programAOSample->setUniform("projectionScale", projectionScale);
		m_programAOSample->setUniform("viewLightPosition", modelViewMatrix*worldLightPosition);
		m_programAOSample->setUniform("surfaceNormalTexture", 0);
		m_programAOSample->setUniform("occlusionIntensity", m_occlusionIntensity);
		m_programAOSample->setUniform("occlusionRadius", m_occlusionRadius);

		m_surfaceNormalTexture->bindActive(0);

		m_vaoQuad->bind();

		m_programAOSample->use();
		m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
		m_programAOSample->release();

		m_vaoQuad->unbind();

		m_aoFramebuffer->unbind();
		//glMemoryBarrier(GL_ALL_BARRIER_BITS);

		m_aoBlurFramebuffer->bind();
		m_programAOBlur->setUniform("normalTexture", 0);
		m_programAOBlur->setUniform("ambientTexture", 1);
		m_programAOBlur->setUniform("offset", vec2(1.0f/float(viewportSize.x),0.0f));

		m_ambientTexture->bindActive(1);

		m_vaoQuad->bind();

		m_programAOBlur->use();
		m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
		m_programAOBlur->release();

		m_vaoQuad->unbind();
		
		m_ambientTexture->unbindActive(1);
		m_aoBlurFramebuffer->unbind();
		//glMemoryBarrier(GL_ALL_BARRIER_BITS);

		m_aoFramebuffer->bind();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		m_programAOBlur->setUniform("offset", vec2(0.0f,1.0f / float(viewportSize.y)));

		m_blurTexture->bindActive(1);

		m_vaoQuad->bind();

		m_programAOBlur->use();
		m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
		m_programAOBlur->release();

		m_vaoQuad->unbind();

		m_blurTexture->unbindActive(1);
		m_surfaceNormalTexture->unbindActive(0);

		m_aoFramebuffer->unbind();
		//glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	m_shadeFramebuffer->bind();
	glDepthMask(GL_FALSE);

	m_surfacePositionTexture->bindActive(0);
	m_surfaceNormalTexture->bindActive(1);
	m_depthTexture->bindActive(2);
	m_ambientTexture->bindActive(3);
	m_scatterPlotTexture->bindActive(4);

	//m_depthRangeBuffer->bindBase(GL_SHADER_STORAGE_BUFFER, 3);

	m_programShade->setUniform("modelViewMatrix", modelViewMatrix);
	m_programShade->setUniform("modelViewProjection", modelViewProjectionMatrix);
	m_programShade->setUniform("inverseModelViewProjectionMatrix", inverseModelViewProjectionMatrix);
	m_programShade->setUniform("normalMatrix", normalMatrix);
	m_programShade->setUniform("inverseNormalMatrix", inverseNormalMatrix);

	// GUI dependent color selection
	m_programShade->setUniform("samplePointColor", viewer()->samplePointColor());
	m_programShade->setUniform("backgroundColor", viewer()->backgroundColor());
	m_programShade->setUniform("contourLineColor", viewer()->contourLineColor());
	m_programShade->setUniform("lensBorderColor", viewer()->lensBorderColor());

	m_programShade->setUniform("lightPosition", vec3(worldLightPosition));
	m_programShade->setUniform("ambientMaterial", ambientMaterial);
	m_programShade->setUniform("diffuseMaterial", diffuseMaterial);
	m_programShade->setUniform("specularMaterial", specularMaterial);
	m_programShade->setUniform("shininess", shininess);
	m_programShade->setUniform("focusPosition", focusPosition);
	m_programShade->setUniform("opacityScale", m_opacityScale);


	m_programShade->setUniform("surfacePositionTexture", 0);
	m_programShade->setUniform("surfaceNormalTexture", 1);
	m_programShade->setUniform("depthTexture", 2);
	m_programShade->setUniform("ambientTexture", 3);
	m_programShade->setUniform("scatterPlotTexture", 4);

	if (m_colorMapLoaded)
	{
		m_colorMapTexture->bindActive(5);
		m_programShade->setUniform("colorMapTexture", 5);
		m_programShade->setUniform("textureWidth", m_ColorMapWidth);
	}

	if (m_blendingFunction == 2)	// DISTANCEBLENDING
	{
		m_kernelDensityTexture->bindActive(6);
		m_programShade->setUniform("kernelDensityTexture", 6);
	}

	if (m_countourLines)
	{
		m_programShade->setUniform("contourCount", m_contourLinesCount);
		m_programShade->setUniform("contourThickness", m_contourThickness);
	}

	// focus and context -> draw lens border
	m_programShade->setUniform("focusPosition", focusPosition);
	m_programShade->setUniform("lensSize", m_lensSize);

	// pass aspect ratio to the shader to make sure lens is always a circle and does not degenerate to an ellipse
	m_programShade->setUniform("aspectRatio", viewer()->m_windowHeight / viewer()->m_windowWidth);
	m_programShade->setUniform("windowHeight", viewer()->m_windowHeight);
	m_programShade->setUniform("windowWidth", viewer()->m_windowWidth);

	m_vaoQuad->bind();

	m_programShade->use();
	m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
	m_programShade->release();

	m_vaoQuad->unbind();

	m_depthRangeBuffer->unbind(GL_SHADER_STORAGE_BUFFER);

	if (m_blendingFunction == 2)	// DISTANCEBLENDING
	{
		m_kernelDensityTexture->unbindActive(6);
	}

	if (m_colorMapLoaded)
	{
		m_colorMapTexture->unbindActive(5);
	}

	m_scatterPlotTexture->unbindActive(4);
	m_ambientTexture->unbindActive(3);
	m_depthTexture->unbindActive(2);
	m_surfaceNormalTexture->unbindActive(1);
	m_surfacePositionTexture->unbindActive(0);

	//glDisable(GL_BLEND);
	m_shadeFramebuffer->unbind();
	
	m_shadeFramebuffer->blit(GL_COLOR_ATTACHMENT0, {0,0,viewer()->viewportSize().x, viewer()->viewportSize().y}, Framebuffer::defaultFBO().get(), GL_BACK, { 0,0,viewer()->viewportSize().x, viewer()->viewportSize().y }, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);


	/*if (adjust)
	{

		//-----------------------------------------
		int sizeX = viewportSize.x;
		int sizeY = viewportSize.y;

		//TODO
		unsigned char BACKGROUND = 255;

		std::vector<unsigned char> image(sizeX * sizeY * 4);
		glReadPixels(0, 0, sizeX, sizeY, GL_RGBA, GL_UNSIGNED_BYTE, (void*)&image.front());

		int curMinX = 2560;
		int curMaxX = 0;
		int curMinY = 1377;
		int curMaxY = 0;



		for (uint y = 0; y < sizeY; y++)
		{
			for (uint x = 0; x < sizeX; x++)
			{
				if (image[4 * x + 0 + 4 * y*sizeX] != BACKGROUND && image[4 * x + 1 + 4 * y*sizeX] != BACKGROUND && image[4 * x + 2 + 4 * y*sizeX] != BACKGROUND) {

					if (curMinX > x) {
						curMinX = x;
					}

					if (curMaxX < x) {
						curMaxX = x;
					}

					if (curMinY > y) {
						curMinY = y;
					}

					if (curMaxY < y) {
						curMaxY = y;
					}
				}
			}
		}

		if (curMinX < curMaxX && curMinY < curMaxY)
		{
			vec4 r1(m_minx, m_miny, m_maxx, m_maxy);
			vec4 r2(curMinX, curMinY, curMaxX, curMaxY);




			vec2 originalSize(m_maxx - m_minx + 1, m_maxy - m_miny + 1);

			vec2 currentSize(curMaxX - curMinX + 1, curMaxY - curMinY + 1);
			vec2 sizeRatio = originalSize / currentSize;

			float scale = std::max(sizeRatio.x, sizeRatio.y);

			//std::cout << "O: " << to_string(originalSize) << std::endl;
			//std::cout << "C: " << to_string(currentSize) << std::endl;
			//std::cout << "R1: " << to_string(r1) << std::endl;
			//std::cout << "R2: " << to_string(r2) << std::endl;


			mat4 viewTransform = viewer()->viewTransform();
			viewTransform = glm::scale(viewTransform, vec3(scale, scale, scale));
			viewer()->setViewTransform(viewTransform);

			//save camera matrix
			glm:mat4 currentCamerMatrix = viewer()->viewTransform();

			std::ofstream myfile;
			myfile.open("C:/Users/thoma/OneDrive/Desktop/PhD Bergen/Papers/Paper 1/Matlab/UserStudyPrep/Output/" + m_fileNames[m_fileDataID].substr(0, m_fileNames[m_fileDataID].length() - 4) + "/" + m_fileNames[m_fileDataID].substr(0, m_fileNames[m_fileDataID].length() - 4) + "_ViewTransform.csv");
			myfile << std::to_string(currentCamerMatrix[0][0]) + ',' + std::to_string(currentCamerMatrix[0][1]) + ',' + std::to_string(currentCamerMatrix[0][2]) + ',' + std::to_string(currentCamerMatrix[0][3]) + "\n";
			myfile << std::to_string(currentCamerMatrix[1][0]) + ',' + std::to_string(currentCamerMatrix[1][1]) + ',' + std::to_string(currentCamerMatrix[1][2]) + ',' + std::to_string(currentCamerMatrix[1][3]) + "\n";
			myfile << std::to_string(currentCamerMatrix[2][0]) + ',' + std::to_string(currentCamerMatrix[2][1]) + ',' + std::to_string(currentCamerMatrix[2][2]) + ',' + std::to_string(currentCamerMatrix[2][3]) + "\n";
			myfile << std::to_string(currentCamerMatrix[3][0]) + ',' + std::to_string(currentCamerMatrix[3][1]) + ',' + std::to_string(currentCamerMatrix[3][2]) + ',' + std::to_string(currentCamerMatrix[3][3]) + "\n";
			myfile.close();


			//adjust = false;
		}		

	}*/
	//-----------------------------------------
	currentState->apply();
}