#pragma once
#include "Tile.h"
#include <memory>

#include <glm/glm.hpp>

namespace molumes
{
	class HexTile : public Tile
	{
	public:
		HexTile(Renderer* renderer);

		globjects::Program * getAccumulationProgram() override;
		globjects::Program * getTileProgram() override;
		globjects::Program * getTileNormalsProgram() override;
		void renderGrid(std::unique_ptr<globjects::VertexArray> const &m_vaoTiles, const glm::mat4 modelViewProjectionMatrix) override;
		void calculateNumberOfTiles(glm::vec3 boundingBoxSize, glm::vec3 minBounds) override;

		int mapPointToTile(glm::vec2 p) override;

	private:

		float horizontal_space = 0.0f;
		float vertical_space = 0.0f;
		float rect_width = 0.0f;
		float rect_height = 0.0f;
		int max_rect_row = 0;
		int max_rect_col = 0;
		glm::vec2 maxBounds_rect;

	};

}

