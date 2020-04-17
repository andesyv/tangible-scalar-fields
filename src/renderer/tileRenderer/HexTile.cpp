#include "HexTile.h"
#include <globjects/base/File.h>
#include <globjects/State.h>
#include <iostream>
#include <omp.h>
#include <lodepng.h>
#include <sstream>

#include <glm/gtc/type_ptr.hpp>

#include <cstdio>
#include <ctime>


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

HexTile::HexTile(Renderer* renderer) :Tile(renderer)
{
	// create shader programs
	renderer->createShaderProgram("hex-acc", {
		{GL_VERTEX_SHADER,"./res/tiles/hexagon/hexagon-acc-vs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/tiles/acc-fs.glsl"}
		});

	renderer->createShaderProgram("hex-tiles", {
		{GL_VERTEX_SHADER,"./res/tiles/image-vs.glsl"},
		{GL_GEOMETRY_SHADER,"./res/tiles/bounding-quad-gs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/tiles/hexagon/hexagon-tiles-fs.glsl"}
		});

	renderer->createShaderProgram("hex-grid", {
		{GL_VERTEX_SHADER,"./res/tiles/hexagon/hexagon-grid-vs.glsl"},
		{GL_GEOMETRY_SHADER,"./res/tiles/hexagon/hexagon-grid-gs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/tiles/grid-fs.glsl"}
		});
}


// --------------------------------------------------------------------------------------
// ###########################  SQUARE ############################################
// --------------------------------------------------------------------------------------

Program * HexTile::getAccumulationProgram()
{
	auto shaderProgram_hex_acc = renderer->shaderProgram("hex-acc");

	shaderProgram_hex_acc->setUniform("maxBounds_rect", maxBounds_hex_rect);
	shaderProgram_hex_acc->setUniform("minBounds_Offset", minBounds_Offset);

	shaderProgram_hex_acc->setUniform("maxTexCoordX", m_tileMaxX);
	shaderProgram_hex_acc->setUniform("maxTexCoordY", m_tileMaxY);

	shaderProgram_hex_acc->setUniform("max_rect_col", hex_max_rect_col);
	shaderProgram_hex_acc->setUniform("max_rect_row", hex_max_rect_row);

	shaderProgram_hex_acc->setUniform("rectHeight", hex_rect_height);
	shaderProgram_hex_acc->setUniform("rectWidth", hex_rect_width);

	return shaderProgram_hex_acc;
}

Program * HexTile::getTileProgram(mat4 modelViewProjectionMatrix, ivec2 viewportSize)
{
	auto shaderProgram_hex_tiles = renderer->shaderProgram("hex-tiles");

	//geometry shader
	shaderProgram_hex_tiles->setUniform("maxBounds_acc", maxBounds_hex_rect);
	shaderProgram_hex_tiles->setUniform("minBounds_acc", minBounds_Offset);

	//geometry & fragment shader
	shaderProgram_hex_tiles->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);

	shaderProgram_hex_tiles->setUniform("windowWidth", viewportSize[0]);
	shaderProgram_hex_tiles->setUniform("windowHeight", viewportSize[1]);
	shaderProgram_hex_tiles->setUniform("rectSize", vec2(hex_rect_width, hex_rect_height));

	//fragment Shader
	shaderProgram_hex_tiles->setUniform("maxTexCoordX", m_tileMaxX);
	shaderProgram_hex_tiles->setUniform("maxTexCoordY", m_tileMaxY);

	shaderProgram_hex_tiles->setUniform("max_rect_col", hex_max_rect_col);
	shaderProgram_hex_tiles->setUniform("max_rect_row", hex_max_rect_row);

	return shaderProgram_hex_tiles;
}

void HexTile::renderGrid(std::unique_ptr<globjects::VertexArray> const &m_vaoTiles, const glm::mat4 modelViewProjectionMatrix)
{
	auto shaderProgram_hexagon_grid = renderer->shaderProgram("hex-grid");
	shaderProgram_hexagon_grid->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);
	shaderProgram_hexagon_grid->setUniform("borderColor", vec3(1.0f, 1.0f, 1.0f));

	shaderProgram_hexagon_grid->setUniform("horizontal_space", hex_horizontal_space);
	shaderProgram_hexagon_grid->setUniform("vertical_space", hex_vertical_space);
	shaderProgram_hexagon_grid->setUniform("num_cols", m_tile_cols);
	shaderProgram_hexagon_grid->setUniform("minBounds_Off", minBounds_Offset);

	shaderProgram_hexagon_grid->setUniform("hexSize", tileSizeWS);
	shaderProgram_hexagon_grid->setUniform("accumulateTexture", 1);

	//draw call
	m_vaoTiles->bind();

	shaderProgram_hexagon_grid->use();
	m_vaoTiles->drawArrays(GL_POINTS, 0, numTiles);
	shaderProgram_hexagon_grid->release();

	m_vaoTiles->unbind();
}

void HexTile::calculateNumberOfTiles(vec3 boundingBoxSize, vec3 minBounds)
{
	// calculations derived from: https://www.redblobgames.com/grids/hexagons/
	// we assume flat topped hexagons
	// we use "Offset Coordinates"
	hex_horizontal_space = tileSizeWS * 1.5f;
	hex_vertical_space = sqrt(3)*tileSizeWS;

	hex_rect_height = hex_vertical_space / 2.0f;
	hex_rect_width = tileSizeWS;

	//number of hex columns
	//+1 because else the floor operation could return 0 
	float cols_tmp = 1 + (boundingBoxSize.x / hex_horizontal_space);
	m_tile_cols = floor(cols_tmp);
	if ((cols_tmp - m_tile_cols) * hex_horizontal_space >= tileSizeWS) {
		m_tile_cols += 1;
	}

	//number of hex rows
	float rows_tmp = 1 + (boundingBoxSize.y / hex_vertical_space);
	m_tile_rows = floor(rows_tmp);
	if ((rows_tmp - m_tile_rows) * hex_vertical_space >= hex_vertical_space / 2) {
		m_tile_rows += 1;
	}

	//max index of rect columns
	if (m_tile_cols % 2 == 1) {
		hex_max_rect_col = ceil(m_tile_cols*1.5f) - 1;
	} 
	else {
		hex_max_rect_col = ceil(m_tile_cols*1.5f);
	}

	//max index of rect rows
	hex_max_rect_row = m_tile_rows * 2;

	//max indices of hexagon grid
	m_tileMaxX = m_tile_cols - 1;
	m_tileMaxY = m_tile_rows - 1;
	numTiles = m_tile_rows * m_tile_cols;

	//bounding box offsets
	minBounds_Offset = vec2(minBounds.x - tileSizeWS / 2.0f, minBounds.y - hex_vertical_space / 2.0f);
	maxBounds_Offset = vec2(m_tile_cols * hex_horizontal_space + minBounds_Offset.x + tileSizeWS / 2.0f, m_tile_rows * hex_vertical_space + minBounds_Offset.y + hex_vertical_space / 2.0f);

	maxBounds_hex_rect = vec2((hex_max_rect_col + 1) * hex_rect_width + minBounds_Offset.x, (hex_max_rect_row + 1)*hex_rect_height + minBounds_Offset.y);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//DISCREPANCY
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
