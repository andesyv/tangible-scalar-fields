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
		std::unique_ptr<globjects::Texture> m_pointCircleTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_hexTilesTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_tilesDiscrepanciesTexture = nullptr; //TODO: could also use free channel of AccumulateTexture
		std::unique_ptr<globjects::Texture> m_squareAccumulateTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_squareTilesTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_squareGridTexture = nullptr;

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
		std::unique_ptr<globjects::Framebuffer> m_squareAccumulateFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_squareTilesFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_squareGridFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_shadeFramebuffer = nullptr;

		glm::ivec2 m_framebufferSize;
		//---------------------------------------------------------------------------------------


		// LIGHTING -----------------------------------------------------------------------
		glm::vec3 ambientMaterial;
		glm::vec3 diffuseMaterial;
		glm::vec3 specularMaterial;
		float shininess;
		// --------------------------------------------------------------------------------

		// VIEWPORT -------------------------------------------------------------------
		//glm::vec4 boundingBoxScreenSpace;
		// --------------------------------------------------------------------------------

		// SQUARE CALC -------------------------------------------------------------------
		int m_squareMaxY = 0;
		int m_squareMaxX = 0;
		int m_squareNumRows = 0; //Y
		int m_squareNumCols = 0; //X
		int numSquares = 0;
		float squareSizeWS = 0.0f;
		const float squareSizeDiv = 500.0f;

		glm::vec2 maxBounds_Offset;

		void calculateSquareTextureSize(const glm::mat4 inverseModelViewProjectionMatrix);

		// HEXAGON CALC -------------------------------------------------------------------

		int m_hexRows = 0;
		int m_hexCols = 0;
		int m_hexCount = 0;
		float horizontal_space = 0.0f;
		float vertical_space = 0.0f;
		float hexSize = 20.0f;
		float hexRot = 0.0f;
		glm::mat4 hexRotMat = glm::mat4(1.0f);

		void renderHexagonGrid(glm::mat4 modelViewProjectionMatrix);
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

		// Square Parameters
		float m_squareSize = 20.0f;
		float m_squareSize_tmp = m_squareSize;

		// Hexagon Parameters
		float m_hexSize_tmp = hexSize;

		float m_hexRot_tmp = hexRot;

		//point circle parameters
		bool m_renderPointCircles = false;
		float m_pointCircleRadius = 50.0f;
		const float pointCircleRadiusDiv = 10000.0f;

		// define booleans
		bool m_renderSquares = false;
		bool m_renderGrid = false;
		bool m_renderAccumulatePoints = false;
		// ------------------------------------------------------------------------------------------

		// INTERVAL MAPPING--------------------------------------------------------------------------
		//maps value x from [a,b] --> [0,c]
		float mapInterval(float x, float a, float b, int c);

		// DISCREPANCY------------------------------------------------------------------------------

		std::vector<float> CalculateDiscrepancy2D(const std::vector<float>& samplesX, const std::vector<float>& samplesY, glm::vec3 maxBounds, glm::vec3 minBounds);
	};

}