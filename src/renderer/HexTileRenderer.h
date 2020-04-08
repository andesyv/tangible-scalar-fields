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

		// TEXTURES -------------------------------------------------------------------------
		std::unique_ptr<globjects::Texture> m_depthTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_colorTexture = nullptr;

		std::unique_ptr<globjects::Texture> m_pointChartTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_pointCircleTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_tilesDiscrepanciesTexture = nullptr; //TODO: could also use free channel of AccumulateTexture
		std::unique_ptr<globjects::Texture> m_tileAccumulateTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_squareTilesTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_gridTexture = nullptr;

		int m_ColorMapWidth = 0;
		std::unique_ptr<globjects::Texture> m_colorMapTexture = nullptr;
		//---------------------------------------------------------------------------------------

		//SSBO
		std::unique_ptr<globjects::Buffer> m_valueMaxBuffer = std::make_unique<globjects::Buffer>();

		// FRAMEBUFFER -------------------------------------------------------------------------
		std::unique_ptr<globjects::Framebuffer> m_pointFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_pointCircleFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_hexFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_tilesDiscrepanciesFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_tileAccumulateFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_squareTilesFramebuffer = nullptr;
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
		const float tileSizeDiv = 500.0f;
		float m_tileSize = 20.0f;
		float tileSizeWS = 0.0f;

		int m_tileMaxY = 0;
		int m_tileMaxX = 0;
		int m_tile_rows = 0; //Y
		int m_tile_cols = 0; //X
		int numTiles = 0;

		glm::vec2 maxBounds_Offset;
		glm::vec2 minBounds_Offset;

		void calculateTileTextureSize(const glm::mat4 inverseModelViewProjectionMatrix);

		// SQUARE CALC -------------------------------------------------------------------

		globjects::Program * getSquareAccumulationProgram();
		globjects::Program * getSquareTileProgram(glm::mat4 modelViewProjectionMatrix, glm::vec2 minBounds);
		void renderSquareGrid(const glm::mat4 modelViewProjectionMatrix);
		void calculateNumberOfSquares(glm::vec3 boundingBoxSize, glm::vec3 minBounds);

		// HEXAGON CALC -------------------------------------------------------------------

		float hex_horizontal_space = 0.0f;
		float hex_vertical_space = 0.0f;
		float hex_width = 0.0f;
		float hex_height = 0.0f;
		float hex_rect_width = 0.0f;
		float hex_rect_height = 0.0f;
		int hex_max_rect_row = 0;
		int hex_max_rect_col = 0;
		glm::vec2 maxBounds_hex_rect;

		globjects::Program * getHexagonAccumulationProgram(glm::vec2 minBounds);
		globjects::Program * getHexagonTileProgram(glm::mat4 modelViewProjectionMatrix, glm::vec2 minBounds);
		void renderHexagonGrid(glm::mat4 modelViewProjectionMatrix, glm::vec2 minBounds);
		void calculateNumberOfHexagons(glm::vec3 boundingBoxSize, glm::vec3 minBounds);

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
		float m_pointCircleRadius = 50.0f;
		const float pointCircleRadiusDiv = 10000.0f;

		//discrepancy parameters
		float m_discrepancyDiv = 1.5f;
		bool m_discrepancy_easeIn = false;

		// define booleans
		bool m_renderPointCircles = true;
		bool m_renderDiscrepancy = false;
		bool m_renderDiscrepancy_tmp = m_renderDiscrepancy;
		bool m_renderGrid = false;
		bool m_renderAccumulatePoints = false;
		bool m_discrepancy_easeIn_tmp = m_discrepancy_easeIn;
		// ------------------------------------------------------------------------------------------

		// INTERVAL MAPPING--------------------------------------------------------------------------
		//maps value x from [a,b] --> [0,c]
		float mapInterval(float x, float a, float b, int c);

		// DISCREPANCY------------------------------------------------------------------------------

		std::vector<float> calculateDiscrepancy2D(const std::vector<float>& samplesX, const std::vector<float>& samplesY, glm::vec3 maxBounds, glm::vec3 minBounds);
		double quadricEaseIn(double t, int b, int c, int d);
	};

}