#include "SquareTile.h"
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

SquareTile::SquareTile(Renderer* renderer) :Tile(renderer)
{
	// create shader programs
	renderer->createShaderProgram("square-acc", {
		{GL_VERTEX_SHADER,"./res/tiles/square/square-acc-vs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/tiles/acc-fs.glsl"}
		});

	renderer->createShaderProgram("square-tiles", {
		{GL_VERTEX_SHADER,"./res/tiles/image-vs.glsl"},
		{GL_GEOMETRY_SHADER,"./res/tiles/bounding-quad-gs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/tiles/square/square-tiles-fs.glsl"}
		});

	renderer->createShaderProgram("square-grid", {
		{GL_VERTEX_SHADER,"./res/tiles/square/square-grid-vs.glsl"},
		{GL_GEOMETRY_SHADER,"./res/tiles/square/square-grid-gs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/tiles/grid-fs.glsl"}
		});
}


// --------------------------------------------------------------------------------------
// ###########################  SQUARE ############################################
// --------------------------------------------------------------------------------------

Program * SquareTile::getAccumulationProgram()
{
	auto shaderProgram_square_acc = renderer->shaderProgram("square-acc");

	shaderProgram_square_acc->setUniform("maxBounds_Off", maxBounds_Offset);
	shaderProgram_square_acc->setUniform("minBounds_Off", minBounds_Offset);

	shaderProgram_square_acc->setUniform("maxTexCoordX", m_tileMaxX);
	shaderProgram_square_acc->setUniform("maxTexCoordY", m_tileMaxY);

	return shaderProgram_square_acc;
}

Program * SquareTile::getTileProgram(mat4 modelViewProjectionMatrix, ivec2 viewportSize)
{
	auto shaderProgram_square_tiles = renderer->shaderProgram("square-tiles");

	//geometry shader
	shaderProgram_square_tiles->setUniform("maxBounds_acc", maxBounds_Offset);
	shaderProgram_square_tiles->setUniform("minBounds_acc", minBounds_Offset);

	//geometry & fragment shader
	shaderProgram_square_tiles->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);

	shaderProgram_square_tiles->setUniform("windowWidth", viewportSize[0]);
	shaderProgram_square_tiles->setUniform("windowHeight", viewportSize[1]);

	//fragment Shader
	shaderProgram_square_tiles->setUniform("maxTexCoordX", m_tileMaxX);
	shaderProgram_square_tiles->setUniform("maxTexCoordY", m_tileMaxY);

	return shaderProgram_square_tiles;
}

void SquareTile::renderGrid(std::unique_ptr<globjects::VertexArray> const &m_vaoTiles, const glm::mat4 modelViewProjectionMatrix)
{
	auto shaderProgram_square_grid = renderer->shaderProgram("square-grid");

	//set uniforms

	//vertex shader
	shaderProgram_square_grid->setUniform("numCols", m_tile_cols);
	shaderProgram_square_grid->setUniform("numRows", m_tile_rows);

	//geometry shader
	shaderProgram_square_grid->setUniform("squareSize", tileSizeWS);
	shaderProgram_square_grid->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);
	shaderProgram_square_grid->setUniform("maxBounds_Off", maxBounds_Offset);
	shaderProgram_square_grid->setUniform("minBounds_Off", minBounds_Offset);
	shaderProgram_square_grid->setUniform("accumulateTexture", 1);

	//fragment Shader
	shaderProgram_square_grid->setUniform("borderColor", vec3(1.0f, 1.0f, 1.0f));
	shaderProgram_square_grid->setUniform("gridTexture", 0);

	//draw call
	m_vaoTiles->bind();

	shaderProgram_square_grid->use();
	m_vaoTiles->drawArrays(GL_POINTS, 0, numTiles);
	shaderProgram_square_grid->release();

	m_vaoTiles->unbind();
}

void SquareTile::calculateNumberOfTiles(vec3 boundingBoxSize, vec3 minBounds)
{
	// get maximum value of X,Y in accumulateTexture-Space
	m_tile_cols = ceil(boundingBoxSize.x / tileSizeWS);
	m_tile_rows = ceil(boundingBoxSize.y / tileSizeWS);
	numTiles = m_tile_rows * m_tile_cols;
	m_tileMaxX = m_tile_cols - 1;
	m_tileMaxY = m_tile_rows - 1;

	//The squares on the maximum sides of the bounding box, will not fit into the box perfectly most of the time
	//therefore we calculate new maximum bounds that fit them perfectly
	//this way we can perform a mapping using the set square size
	maxBounds_Offset = vec2(m_tile_cols * tileSizeWS + minBounds.x, m_tile_rows * tileSizeWS + minBounds.y);
	minBounds_Offset = minBounds;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//DISCREPANCY
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//maps value x from [a,b] --> [0,c]
float SquareTile::mapInterval(float x, float a, float b, int c)
{
	return (x - a)*c / (b - a);
}