#version 450
#extension GL_ARB_shading_language_include : require
#include "/defines.glsl"
#include "/globals.glsl"

layout(location = 0) in float xValue;

uniform int numCols;
uniform int numRows;
//[x,y]
uniform vec2 maxBounds_Off;
uniform vec2 minBounds;

void main()
{
	//calculate position of square center
	float col = mod(gl_VertexID,numCols);
    float row = gl_VertexID/numRows;

    // map from square space to bounding box space
    float bbX = mapInterval_O(col, 0, numCols, minBounds[0], maxBounds_Off[0]);
    float bbY = mapInterval_O(row, 0, numRows, minBounds[1], maxBounds_Off[1]);
	
	gl_Position =  vec4(bbX, bbY, 0.0f, 1.0f);
}