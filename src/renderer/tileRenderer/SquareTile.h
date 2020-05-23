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
		globjects::Program * getTileProgram() override;
		globjects::Program * getTileNormalsProgram() override;
		globjects::Program * getGridProgram() override;

		void calculateNumberOfTiles(glm::vec3 boundingBoxSize, glm::vec3 minBounds) override;

		int mapPointToTile(glm::vec2 p) override;

	private:

	};

}

