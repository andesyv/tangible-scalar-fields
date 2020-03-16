#version 450
#extension GL_ARB_shading_language_include : require
#include "/defines.glsl"
#include "/globals.glsl"

layout(location = 1) in float discrepancy;

uniform int numCols;
uniform int numRows;
uniform float discrepancyDiv;

out vec4 tileDiscrepancy;

void main()
{
	//calculate position of square center
	float col = mod(gl_VertexID,numCols);
    float row = gl_VertexID/numCols;

    // we only set the red channel, because we only use the color for additive blending
    tileDiscrepancy = vec4(1-discrepancy/discrepancyDiv,0.0f,0.0f,1.0f);

    // debug: color point according to square
    //tileDiscrepancy = vec4(float(col/float(numCols)),float(row/float(numRows)),0.0f,1.0f);

    // NDC - map square coordinates to [-1,1] (first [0,2] than -1)
	vec2 squareNDC = vec2(((col * 2) / float(numCols)) - 1, ((row * 2) / float(numRows)) - 1);

    gl_Position = vec4(squareNDC, 0.0f, 1.0f);
}