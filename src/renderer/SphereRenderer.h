#pragma once
#include "Renderer.h"
#include <memory>

#include <glm/glm.hpp>
#include <glbinding/gl/gl.h>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>

#include <globjects/VertexArray.h>
#include <globjects/VertexAttributeBinding.h>
#include <globjects/Buffer.h>
#include <globjects/Program.h>
#include <globjects/Shader.h>
#include <globjects/Framebuffer.h>
#include <globjects/Renderbuffer.h>
#include <globjects/Texture.h>
#include <globjects/base/File.h>
#include <globjects/TextureHandle.h>
#include <globjects/NamedString.h>
#include <globjects/base/StaticStringSource.h>
#include <globjects/Query.h>

namespace molumes
{
	class Viewer;

	class SphereRenderer : public Renderer
	{
	public:
		SphereRenderer(Viewer *viewer);
		virtual void display();
		virtual std::list<globjects::File*> shaderFiles() const;

	private:
		
		std::unique_ptr<globjects::VertexArray> m_vao = std::make_unique<globjects::VertexArray>();
		std::unique_ptr<globjects::Buffer> m_xColumnBuffer = std::make_unique<globjects::Buffer>();
		std::unique_ptr<globjects::Buffer> m_yColumnBuffer = std::make_unique<globjects::Buffer>();
		std::unique_ptr<globjects::Buffer> m_radiusColumnBuffer = std::make_unique<globjects::Buffer>();
		std::unique_ptr<globjects::Buffer> m_colorColumnBuffer = std::make_unique<globjects::Buffer>();
		
		std::unique_ptr<globjects::VertexArray> m_vaoQuad = std::make_unique<globjects::VertexArray>();
		std::unique_ptr<globjects::Buffer> m_verticesQuad = std::make_unique<globjects::Buffer>();
		
		std::unique_ptr<globjects::Program> m_programSphere = std::make_unique<globjects::Program>();
		std::unique_ptr<globjects::Program> m_programSpawn = std::make_unique<globjects::Program>();
		std::unique_ptr<globjects::Program> m_programDensityEstimation = std::make_unique<globjects::Program>();
		std::unique_ptr<globjects::Program> m_programGeomMean = std::make_unique<globjects::Program>();
		std::unique_ptr<globjects::Program> m_programSurface = std::make_unique<globjects::Program>();
		std::unique_ptr<globjects::Program> m_programAOSample = std::make_unique<globjects::Program>();
		std::unique_ptr<globjects::Program> m_programAOBlur = std::make_unique<globjects::Program>();
		std::unique_ptr<globjects::Program> m_programShade = std::make_unique<globjects::Program>();

		std::unique_ptr<globjects::StaticStringSource> m_shaderSourceDefines = nullptr;
		std::unique_ptr<globjects::NamedString> m_shaderDefines = nullptr;

		std::unique_ptr<globjects::File> m_shaderSourceGlobals = nullptr;
		std::unique_ptr<globjects::NamedString> m_shaderGlobals = nullptr;

		std::unique_ptr<globjects::File> m_vertexShaderSourceSphere = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_vertexShaderTemplateSphere = nullptr;
		std::unique_ptr<globjects::Shader> m_vertexShaderSphere = nullptr;

		std::unique_ptr<globjects::File> m_geometryShaderSourceSphere = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_geometryShaderTemplateSphere = nullptr;
		std::unique_ptr<globjects::Shader> m_geometryShaderSphere = nullptr;

		std::unique_ptr<globjects::File> m_fragmentShaderSourceSphere = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_fragmentShaderTemplateSphere = nullptr;
		std::unique_ptr<globjects::Shader> m_fragmentShaderSphere = nullptr;

		std::unique_ptr<globjects::File> m_vertexShaderSourceImage = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_vertexShaderTemplateImage = nullptr;
		std::unique_ptr<globjects::Shader> m_vertexShaderImage = nullptr;

		std::unique_ptr<globjects::File> m_geometryShaderSourceImage = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_geometryShaderTemplateImage = nullptr;
		std::unique_ptr<globjects::Shader> m_geometryShaderImage = nullptr;

		std::unique_ptr<globjects::File> m_fragmentShaderSourceSpawn = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_fragmentShaderTemplateSpawn = nullptr;
		std::unique_ptr<globjects::Shader> m_fragmentShaderSpawn = nullptr;

		std::unique_ptr<globjects::File> m_fragmentShaderSourceSurface = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_fragmentShaderTemplateSurface = nullptr;
		std::unique_ptr<globjects::Shader> m_fragmentShaderSurface = nullptr;

		std::unique_ptr<globjects::File> m_fragmentShaderSourceDensityEstimation = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_fragmentShaderTemplateDensityEstimation = nullptr;
		std::unique_ptr<globjects::Shader> m_fragmentShaderDensityEstimation = nullptr;

		std::unique_ptr<globjects::File> m_fragmentShaderSourceGeomMean = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_fragmentShaderTemplateGeomMean = nullptr;
		std::unique_ptr<globjects::Shader> m_fragmentShaderGeomMean = nullptr;

		std::unique_ptr<globjects::File> m_vertexShaderSourceGeomMean = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_vertexShaderTemplateGeomMean = nullptr;
		std::unique_ptr<globjects::Shader> m_vertexShaderGeomMean = nullptr;

		std::unique_ptr<globjects::File> m_fragmentShaderSourceAOSample = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_fragmentShaderTemplateAOSample = nullptr;
		std::unique_ptr<globjects::Shader> m_fragmentShaderAOSample = nullptr;

		std::unique_ptr<globjects::File> m_fragmentShaderSourceAOBlur = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_fragmentShaderTemplateAOBlur = nullptr;
		std::unique_ptr<globjects::Shader> m_fragmentShaderAOBlur = nullptr;

		std::unique_ptr<globjects::File> m_fragmentShaderSourceShade = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_fragmentShaderTemplateShade = nullptr;
		std::unique_ptr<globjects::Shader> m_fragmentShaderShade = nullptr;

		std::unique_ptr<globjects::Buffer> m_intersectionBuffer = std::make_unique<globjects::Buffer>();
		std::unique_ptr<globjects::Buffer> m_statisticsBuffer = std::make_unique<globjects::Buffer>();
		std::unique_ptr<globjects::Buffer> m_depthRangeBuffer = std::make_unique<globjects::Buffer>();
		std::unique_ptr<globjects::Buffer> m_geomMeanBuffer = std::make_unique<globjects::Buffer>();

		int m_ColorMapWidth = 0;
		std::unique_ptr<globjects::Texture> m_colorMapTexture = nullptr;

		std::unique_ptr<globjects::Texture> m_offsetTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_depthTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_spherePositionTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_sphereNormalTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_surfacePositionTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_surfaceNormalTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_sphereDiffuseTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_surfaceDiffuseTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_ambientTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_blurTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_colorTexture = nullptr;

		// necessary to make rendered dataset exactly the same size as used in user study------
		//int m_minx, m_maxx, m_miny, m_maxy;
		//-------------------------------------------------------------------------------------

		std::unique_ptr<globjects::Texture> m_scatterPlotTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_pilotKernelDensityTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_kernelDensityTexture = nullptr;

		std::unique_ptr<globjects::Framebuffer> m_sphereFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_surfaceFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_ambientFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_shadeFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_aoFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_aoBlurFramebuffer = nullptr;


		std::unique_ptr<globjects::Query> m_sphereQuery = nullptr;
		std::unique_ptr<globjects::Query> m_spawnQuery = nullptr;
		std::unique_ptr<globjects::Query> m_surfaceQuery = nullptr;
		std::unique_ptr<globjects::Query> m_shadeQuery = nullptr;
		std::unique_ptr<globjects::Query> m_totalQuery = nullptr;
		glm::uint m_benchmarkCounter = 0;
		glm::uint m_benchmarkPhase = 0;
		glm::uint m_benchmarkIndex = 0;
		bool m_benchmarkWarmup = true;

		glm::ivec2 m_framebufferSize;

		// GUI variables ----------------------------------------------------------------------------

		// items for ImGui Combo
		std::string m_guiFileNames = { 'N', 'o', 'n', 'e', '\0' };
		std::vector<std::string>  m_fileNames = { "None" };

		std::string m_guiColumnNames = { 'N', 'o', 'n', 'e', '\0' };

		// store combo ID of selected file
		int m_fileDataID = 0;
		int m_oldFileDataID = 0;
		
		// store combo ID of selected columns
		int m_xAxisDataID = 0, m_yAxisDataID = 0, m_radiusDataID = 0, m_colorDataID = 0;

		// selection of color maps
		int m_colorMap = 8;						// use "plasma" as default heatmap
		bool m_colorMapLoaded = false;
		bool m_heatMapGUI = false;
		bool m_discreteMap = false;
		// ---------------------------------
		int m_oldColorMap = 0;
		bool m_oldDiscreteMap = false;

		// illumination selections
		bool m_surfaceIllumination = false;
		bool m_ambientOcclusion = false;

		float m_occlusionIntensity = 1.0f;
		float m_occlusionRadius = 0.2f;
		
		// GUI-slide used for additional scaling values (empirically chosen)
		float m_radiusMultiplier = 55.0f;
		float m_sigma = 20.0f;
		float m_gaussScale = 0.1f;
		float m_scatterScale = 5.0f;
		float m_opacityScale = 0.8f;

		// selection of blending function
		int m_blendingFunction = 0;
		int m_colorScheme = 0;
		bool m_invertFunction = false;

		// contour lines
		int m_contourLinesCount = 7;
		float m_contourThickness = 0.02f;
		bool m_countourLines = false;

		// adaptive kernel size
		bool m_adaptKernel = false;

		// adaptive kernel density estimation
		bool m_adaptiveKDE = false;
		float m_alpha = 0.5f;

		// focus and context
		float m_lensSize = 0.25f;
		float m_lensSigma = 0.2f;
		bool m_lenseEnabled = false;

		// ------------------------------------------------------------------------------------------
	};

}