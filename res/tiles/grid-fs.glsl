#version 450
#include "/defines.glsl"
#include "/globals.glsl"

layout (location = 0) out vec4 gridTexture;

uniform vec3 borderColor;

in vec2 isCoords;
in float tileSizeScreenSpace;
in vec2 tileCenterScreenSpace;

uniform int windowHeight;
uniform int windowWidth;

void main()
{

	float x = (tileCenterScreenSpace.x + tileSizeScreenSpace) - gl_FragCoord.x;

	if (x < 1){
		gridTexture = vec4(1,0,0, 1.0f);
	}
	else{
		gridTexture = vec4(borderColor, 1.0f);
	}	
}