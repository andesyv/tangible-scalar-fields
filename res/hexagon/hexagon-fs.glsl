#version 450
#include "/defines.glsl"

layout (location = 0) out vec4 hexTilesTexture;

uniform vec3 pointColor;

void main()
{
	hexTilesTexture = vec4(pointColor, 1.0f);
}