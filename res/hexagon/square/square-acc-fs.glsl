#version 450
#include "/defines.glsl"
#include "/globals.glsl"

//out
layout (location = 0) out vec4 squareAccumulateTexture;

in vec4 vertexColor;

void main(){

	squareAccumulateTexture = vertexColor;
}