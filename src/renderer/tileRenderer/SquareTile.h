#pragma once
#include "Tile.h"
#include <memory>

#include <glm/glm.hpp>

namespace molumes
{
	class SquareTile : public Tile
	{
	public:
		SquareTile(Renderer* renderer);

		globjects::Program * getAccumulationProgram() override;
		globjects::Program * getTileProgram(glm::mat4 modelViewProjectionMatrix, glm::ivec2 viewportSize) override;

		void renderGrid(std::unique_ptr<globjects::VertexArray> const &m_vaoTiles, const glm::mat4 modelViewProjectionMatrix) override;

		void calculateNumberOfTiles(glm::vec3 boundingBoxSize, glm::vec3 minBounds) override;

		int mapPointToTile(glm::vec2 p) override;

	private:

	};

}

