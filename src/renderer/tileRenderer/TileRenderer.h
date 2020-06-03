#pragma once
#include "../Renderer.h"
#include "Tile.h"
#include "SquareTile.h"
#include "HexTile.h"
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

	class TileRenderer : public Renderer
	{
	public:
		TileRenderer(Viewer *viewer);
		~TileRenderer();
		virtual void display();

	private:

		Tile* tile = nullptr;
		std::unordered_map<std::string, Tile*> tile_processors;

		// INITIAL POINT DATA ----------------------------------------------------------------------
		std::unique_ptr<globjects::VertexArray> m_vao = std::make_unique<globjects::VertexArray>();
		std::unique_ptr<globjects::Buffer> m_xColumnBuffer = std::make_unique<globjects::Buffer>();
		std::unique_ptr<globjects::Buffer> m_yColumnBuffer = std::make_unique<globjects::Buffer>();
		std::unique_ptr<globjects::Buffer> m_radiusColumnBuffer = std::make_unique<globjects::Buffer>();
		std::unique_ptr<globjects::Buffer> m_colorColumnBuffer = std::make_unique<globjects::Buffer>();

		//---------------------------------------------------------------------------------------

		// TILES GRID VERTEX DATA------------------------------------------------------------
		std::unique_ptr<globjects::VertexArray> m_vaoTiles = std::make_unique<globjects::VertexArray>();
		std::unique_ptr<globjects::Buffer> m_verticesTiles = std::make_unique<globjects::Buffer>();
		std::unique_ptr<globjects::Buffer> m_discrepanciesBuffer = std::make_unique<globjects::Buffer>();

		//---------------------------------------------------------------------------------------

		// QUAD VERTEX DATA -------------------------------------------------------------------------------
		std::unique_ptr<globjects::VertexArray> m_vaoQuad = std::make_unique<globjects::VertexArray>();
		std::unique_ptr<globjects::Buffer> m_verticesQuad = std::make_unique<globjects::Buffer>();
		//---------------------------------------------------------------------------------------

		std::unique_ptr<globjects::StaticStringSource> m_shaderSourceDefines = nullptr;
		std::unique_ptr<globjects::NamedString> m_shaderDefines = nullptr;

		std::unique_ptr<globjects::File> m_shaderSourceGlobals = nullptr;
		std::unique_ptr<globjects::NamedString> m_shaderGlobals = nullptr;

		std::unique_ptr<globjects::File> m_shaderSourceGlobalsHexagon = nullptr;
		std::unique_ptr<globjects::NamedString> m_shaderGlobalsHexagon = nullptr;

		// TEXTURES -------------------------------------------------------------------------
		std::unique_ptr<globjects::Texture> m_depthTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_colorTexture = nullptr;

		std::unique_ptr<globjects::Texture> m_pointChartTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_pointCircleTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_tilesDiscrepanciesTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_tileAccumulateTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_tilesTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_gridTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_kdeTexture = nullptr;

		int m_ColorMapWidth = 0;
		std::unique_ptr<globjects::Texture> m_colorMapTexture = nullptr;
		//---------------------------------------------------------------------------------------

		//SSBO
		std::unique_ptr<globjects::Buffer> m_valueMaxBuffer = std::make_unique<globjects::Buffer>();
		std::unique_ptr<globjects::Buffer> m_tileNormalsBuffer = std::make_unique<globjects::Buffer>();

		// FRAMEBUFFER -------------------------------------------------------------------------
		std::unique_ptr<globjects::Framebuffer> m_pointFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_pointCircleFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_hexFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_tilesDiscrepanciesFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_tileAccumulateFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_tilesFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_gridFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_shadeFramebuffer = nullptr;

		glm::ivec2 m_framebufferSize;
		//---------------------------------------------------------------------------------------


		// LIGHTING -----------------------------------------------------------------------
		glm::vec3 ambientMaterial;
		glm::vec3 diffuseMaterial;
		glm::vec3 specularMaterial;
		float shininess;

		// --------------------------------------------------------------------------------

		// TILES CALC----------------------------------------------------------------
		float m_tileSize = 20.0f;

		void calculateTileTextureSize(const glm::mat4 inverseModelViewProjectionMatrix);

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

		// Tiles Parameters
		// [none=0, square=1, hexagon=2]
		int m_selected_tile_style = 0;
		int m_selected_tile_style_tmp = m_selected_tile_style;

		// Tile Parameters
		float m_tileSize_tmp = m_tileSize;

		//point circle parameters 
		float m_pointCircleRadius = 30.0f;
		float m_kdeRadius = 30.0f;
		const float pointCircleRadiusDiv = 5000.0f;

		//discrepancy parameters
		float m_discrepancyDiv = 1.5f;
		float m_discrepancy_easeIn = 1.0f;
		float m_discrepancy_lowCount = 0.0f;

		//regression parameters
		float m_sigma = 1.0f;
		const float gaussSampleRadiusMult = 400.0f;
		float m_densityMult = 10.0f;
		float m_tileHeightMult = 1.0f;
		float m_borderWidth = 0.2f; // width in percent to size
		bool m_showBorder = true;
		bool m_invertPyramid = false;
		float blendRange = 1.0f;

		// define booleans
		bool m_renderPointCircles = false;
		bool m_renderDiscrepancy = false;
		bool m_renderDiscrepancy_tmp = m_renderDiscrepancy;
		bool m_renderGrid = false;
		float m_gridWidth = 1.5f;
		bool m_renderKDE = false;
		bool m_renderTileNormals = false;
		float m_discrepancy_easeIn_tmp = m_discrepancy_easeIn;
		float m_discrepancy_lowCount_tmp = m_discrepancy_lowCount;
		// ------------------------------------------------------------------------------------------

		// INTERVAL MAPPING--------------------------------------------------------------------------
		//maps value x from [a,b] --> [0,c]
		float mapInterval(float x, float a, float b, int c);

		// DISCREPANCY------------------------------------------------------------------------------

		std::vector<float> calculateDiscrepancy2D(const std::vector<float>& samplesX, const std::vector<float>& samplesY, glm::vec3 maxBounds, glm::vec3 minBounds);


		int debugOutputCount = 0.0f;
	};

}

