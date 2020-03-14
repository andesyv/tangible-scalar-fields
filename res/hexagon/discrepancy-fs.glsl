#version 450
#include "/defines.glsl"
#include "/globals.glsl"

//out
layout (location = 0) out vec4 tileDiscrepancyTexture;

in float tileDiscrepancy;

void main(){

	tileDiscrepancyTexture = vec4(tileDiscrepancy,0.0f,0.0f,1.0f);
}