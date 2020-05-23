#pragma once
#include "../Renderer.h"
#include <memory>

#include <glm/glm.hpp>

namespace molumes
{
	class Renderer;

	//IS USED AS ABSTRACT CLASS!!
	//DOES NOT CONTAIN ANY IMPLEMENTATIONS
	class Tile
	{
	public:
		Tile(Renderer* renderer);

		// TILE CALC -------------------------------------------------------------------
		virtual globjects::Program * getAccumulationProgram();
		virtual globjects::Program * getTileProgram();
		virtual globjects::Program * getTileNormalsProgram();
		virtual globjects::Program * getGridProgram();
		virtual void calculateNumberOfTiles(glm::vec3 boundingBoxSize, glm::vec3 minBounds);
		virtual int mapPointToTile(glm::vec2 p);

		const float tileSizeDiv = 500.0f;
		float tileSizeWS = 0.0f;

		int m_tileMaxY = 0;
		int m_tileMaxX = 0;
		int m_tile_rows = 0; //Y
		int m_tile_cols = 0; //X
		int numTiles = 0;

		const float normalsFactor = 10000.0f;

		glm::vec2 maxBounds_Offset;
		glm::vec2 minBounds_Offset;

	protected:

		//maps value x from [a,b] --> [0,c]
		int mapInterval(float x, float a, float b, int c);

		// RENDERER REFERENCE -------------------------------------------------------------
		Renderer* renderer;

	private:
	};

}

