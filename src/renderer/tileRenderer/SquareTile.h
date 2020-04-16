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
	class SquareTile : public Tile
	{
	public:
		SquareTile(Renderer* renderer);

		globjects::Program * getAccumulationProgram() override;
		globjects::Program * getTileProgram(glm::mat4 modelViewProjectionMatrix, glm::ivec2 viewportSize) override;
		void renderGrid(std::unique_ptr<globjects::VertexArray> m_vaoTiles, const glm::mat4 modelViewProjectionMatrix) override;
		void calculateNumberOfTiles(glm::vec3 boundingBoxSize, glm::vec3 minBounds) override;

	private:

		//maps value x from [a,b] --> [0,c]
		float mapInterval(float x, float a, float b, int c);

	};

}

