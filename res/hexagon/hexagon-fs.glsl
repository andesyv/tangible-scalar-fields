#version 450
#include "/defines.glsl"

layout (location = 0) out vec4 hexTilesTexture;

uniform vec3 hexBorderColor;

void main()
{
	hexTilesTexture = vec4(hexBorderColor, 1.0f);
}