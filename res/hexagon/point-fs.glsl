#version 450
#include "/defines.glsl"

layout (location = 0) out vec4 pointChartTexture;

uniform vec3 pointColor;

void main()
{
	pointChartTexture = vec4(pointColor, 1.0f);
}