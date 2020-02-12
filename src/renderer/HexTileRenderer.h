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
		//virtual std::list<globjects::File*> shaderFiles() const;

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

		std::unique_ptr<globjects::StaticStringSource> m_shaderSourceDefines = nullptr;
		std::unique_ptr<globjects::NamedString> m_shaderDefines = nullptr;

		std::unique_ptr<globjects::File> m_shaderSourceGlobals = nullptr;
		std::unique_ptr<globjects::NamedString> m_shaderGlobals = nullptr;	

		// TEXTURES -------------------------------------------------------------------------
		std::unique_ptr<globjects::Texture> m_depthTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_colorTexture = nullptr;

		std::unique_ptr<globjects::Texture> m_pointChartTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_hexTilesTexture = nullptr;

		int m_ColorMapWidth = 0;
		std::unique_ptr<globjects::Texture> m_colorMapTexture = nullptr;
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
		float hexSize = 20.0f;
		float hexRot = 0.0f;
		glm::mat4 hexRotMat = glm::mat4(1.0f);

		void calculateNumberOfHexagons();
		void setRotationMatrix();

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
		bool m_discreteMap = false;
		// ---------------------------------
		int m_oldColorMap = 0;
		bool m_oldDiscreteMap = false;

		// Hexagon Parameters
		float m_hexSize_tmp = hexSize;

		float m_hexRot_tmp = hexRot;
		// ------------------------------------------------------------------------------------------
	};

}