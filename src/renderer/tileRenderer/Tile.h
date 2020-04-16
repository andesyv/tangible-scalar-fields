#pragma once
#include "../Renderer.h"
#include <memory>

#include <glm/glm.hpp>
#include <glbinding/gl/gl.h> 
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>

#include <globjects/VertexArray.h>
#include <globjects/VertexAttributeBinding.h>
#include <globjects/Program.h>
#include <globjects/Shader.h>
#include <globjects/Texture.h>
#include <globjects/NamedString.h>
#include <globjects/base/StaticStringSource.h>

namespace molumes
{
	class Renderer;

	class Tile
	{
	public:
		Tile(Renderer* renderer);

		// TILE CALC -------------------------------------------------------------------
		virtual globjects::Program * getAccumulationProgram();
		virtual globjects::Program * getTileProgram(glm::mat4 modelViewProjectionMatrix, glm::ivec2 viewportSize);
		virtual void renderGrid(std::unique_ptr<globjects::VertexArray> m_vaoTiles, const glm::mat4 modelViewProjectionMatrix);
		virtual void calculateNumberOfTiles(glm::vec3 boundingBoxSize, glm::vec3 minBounds);

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

	protected:

		// RENDERER REFERENCE -------------------------------------------------------------
		Renderer* renderer;

	private:
	};

}

