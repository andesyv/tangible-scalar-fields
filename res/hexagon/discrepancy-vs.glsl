#version 450
#extension GL_ARB_shading_language_include : require
#include "/defines.glsl"
#include "/globals.glsl"

layout(location = 1) in float discrepancy;

uniform int numCols;
uniform int numRows;

out float tileDiscrepancy;

void main()
{
	//calculate position of square center
	float col = mod(gl_VertexID,numCols);
    float row = gl_VertexID/numCols;

    tileDiscrepancy = discrepancy;

    // NDC - map square coordinates to [-1,1] (first [0,2] than -1)
	vec2 squareNDC = vec2(((col * 2) / float(numCols)) - 1, ((row * 2) / float(numRows)) - 1);

    gl_Position = vec4(squareNDC, 0.0f, 1.0f);
}