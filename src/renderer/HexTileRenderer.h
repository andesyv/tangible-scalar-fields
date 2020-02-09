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

	class HexTileRenderer : public Renderer
	{
	public:
		HexTileRenderer(Viewer *viewer);
		virtual void display();
		virtual std::list<globjects::File*> shaderFiles() const;

	private:


		// INITIAL POINT DATA ----------------------------------------------------------------------
		std::unique_ptr<globjects::VertexArray> m_vao = std::make_unique<globjects::VertexArray>();
		std::unique_ptr<globjects::Buffer> m_xColumnBuffer = std::make_unique<globjects::Buffer>();
		std::unique_ptr<globjects::Buffer> m_yColumnBuffer = std::make_unique<globjects::Buffer>();
		std::unique_ptr<globjects::Buffer> m_radiusColumnBuffer = std::make_unique<globjects::Buffer>();
		std::unique_ptr<globjects::Buffer> m_colorColumnBuffer = std::make_unique<globjects::Buffer>();
		//---------------------------------------------------------------------------------------

		// HEX VERTEX DATA -------------------------------------------------------------------------------
		std::unique_ptr<globjects::VertexArray> m_vaoHex = std::make_unique<globjects::VertexArray>();
		std::unique_ptr<globjects::Buffer> m_verticesHex = std::make_unique<globjects::Buffer>();
		//---------------------------------------------------------------------------------------

		// QUAD VERTEX DATA -------------------------------------------------------------------------------
		std::unique_ptr<globjects::VertexArray> m_vaoQuad = std::make_unique<globjects::VertexArray>();
		std::unique_ptr<globjects::Buffer> m_verticesQuad = std::make_unique<globjects::Buffer>();
		//---------------------------------------------------------------------------------------

		// SHADER PROGRAMS -------------------------------------------------------------------------
		std::unique_ptr<globjects::Program> m_programPoint = std::make_unique<globjects::Program>();
		std::unique_ptr<globjects::Program> m_programHex = std::make_unique<globjects::Program>();
		std::unique_ptr<globjects::Program> m_programShade = std::make_unique<globjects::Program>();
		//---------------------------------------------------------------------------------------

		std::unique_ptr<globjects::StaticStringSource> m_shaderSourceDefines = nullptr;
		std::unique_ptr<globjects::NamedString> m_shaderDefines = nullptr;

		std::unique_ptr<globjects::File> m_shaderSourceGlobals = nullptr;
		std::unique_ptr<globjects::NamedString> m_shaderGlobals = nullptr;

		// POINT SHADER -------------------------------------------------------------------------
		std::unique_ptr<globjects::File> m_vertexShaderSourcePoint = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_vertexShaderTemplatePoint = nullptr;
		std::unique_ptr<globjects::Shader> m_vertexShaderPoint = nullptr;

		std::unique_ptr<globjects::File> m_fragmentShaderSourcePoint = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_fragmentShaderTemplatePoint = nullptr;
		std::unique_ptr<globjects::Shader> m_fragmentShaderPoint = nullptr;
		//---------------------------------------------------------------------------------------

		// POINT SHADER -------------------------------------------------------------------------
		std::unique_ptr<globjects::File> m_vertexShaderSourceHex = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_vertexShaderTemplateHex = nullptr;
		std::unique_ptr<globjects::Shader> m_vertexShaderHex = nullptr;

		std::unique_ptr<globjects::File> m_geometryShaderSourceHex = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_geometryShaderTemplateHex = nullptr;
		std::unique_ptr<globjects::Shader> m_geometryShaderHex = nullptr;

		std::unique_ptr<globjects::File> m_fragmentShaderSourceHex = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_fragmentShaderTemplateHex = nullptr;
		std::unique_ptr<globjects::Shader> m_fragmentShaderHex = nullptr;
		//---------------------------------------------------------------------------------------

		// QUAD SHADER --------------------------------------------------------------------------
		std::unique_ptr<globjects::File> m_vertexShaderSourceImage = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_vertexShaderTemplateImage = nullptr;
		std::unique_ptr<globjects::Shader> m_vertexShaderImage = nullptr;

		std::unique_ptr<globjects::File> m_geometryShaderSourceImage = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_geometryShaderTemplateImage = nullptr;
		std::unique_ptr<globjects::Shader> m_geometryShaderImage = nullptr;
		//---------------------------------------------------------------------------------------

		// SHADE SHADER -------------------------------------------------------------------------
		std::unique_ptr<globjects::File> m_fragmentShaderSourceShade = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_fragmentShaderTemplateShade = nullptr;
		std::unique_ptr<globjects::Shader> m_fragmentShaderShade = nullptr;
		//---------------------------------------------------------------------------------------

		// TEXTURES -------------------------------------------------------------------------
		std::unique_ptr<globjects::Texture> m_depthTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_colorTexture = nullptr;

		std::unique_ptr<globjects::Texture> m_pointChartTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_hexTilesTexture = nullptr;

		//---------------------------------------------------------------------------------------

		// FRAMEBUFFER -------------------------------------------------------------------------
		std::unique_ptr<globjects::Framebuffer> m_pointFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_hexFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_shadeFramebuffer = nullptr;

		glm::ivec2 m_framebufferSize;
		//---------------------------------------------------------------------------------------


		// LIGHTING -----------------------------------------------------------------------
		glm::vec3 ambientMaterial;
		glm::vec3 diffuseMaterial;
		glm::vec3 specularMaterial;
		float shininess;
		// --------------------------------------------------------------------------------

		// HEXAGON CALC -------------------------------------------------------------------

		int m_hexRows = 0;
		int m_hexCols = 0;
		int m_hexCount = 0;
		float horizontal_space = 0.0f;
		float vertical_space = 0.0f;

		void calculateNumberOfHexagons();

		// ------------------------------------------------------------------------------------------

		// GUI ----------------------------------------------------------------------------

		void renderGUI();
		void setShaderDefines();

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

		// Hexagon Parameters
		float m_hexSize = 20.0f;
		float m_hexSize_tmp = m_hexSize;

		float m_hexRot = 0.0f;
		// ------------------------------------------------------------------------------------------
	};

}