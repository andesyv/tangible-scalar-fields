#pragma once
#include "Tile.h"
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
#include <globjects/TextureHandle.h>

namespace molumes
{
	class HexTile : public Tile
	{
	public:
		HexTile(Renderer* renderer);

		globjects::Program * getAccumulationProgram() override;
		globjects::Program * getTileProgram(glm::mat4 modelViewProjectionMatrix, glm::ivec2 viewportSize) override;
		void renderGrid(std::unique_ptr<globjects::VertexArray> m_vaoTiles, const glm::mat4 modelViewProjectionMatrix) override;
		void calculateNumberOfTiles(glm::vec3 boundingBoxSize, glm::vec3 minBounds) override;

		float hex_horizontal_space = 0.0f;
		float hex_vertical_space = 0.0f;
		float hex_rect_width = 0.0f;
		float hex_rect_height = 0.0f;
		int hex_max_rect_row = 0;
		int hex_max_rect_col = 0;
		glm::vec2 maxBounds_hex_rect;

	private:


	};

}

