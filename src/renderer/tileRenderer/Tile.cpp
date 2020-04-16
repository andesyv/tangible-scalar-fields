#include "Tile.h"

using namespace molumes;

//IS USED AS ABSTRACT CLASS!!
//DOES NOT CONTAIN ANY IMPLEMENTATIONS
molumes::Tile::Tile(Renderer* renderer)
{
	this->renderer = renderer;
}

globjects::Program * molumes::Tile::getAccumulationProgram()
{
	return nullptr;
}

globjects::Program * molumes::Tile::getTileProgram(glm::mat4 modelViewProjectionMatrix, glm::ivec2 viewportSize)
{
	return nullptr;
}

void molumes::Tile::renderGrid(std::unique_ptr<globjects::VertexArray> m_vaoTiles, const glm::mat4 modelViewProjectionMatrix)
{
}

void molumes::Tile::calculateNumberOfTiles(glm::vec3 boundingBoxSize, glm::vec3 minBounds)
{
}