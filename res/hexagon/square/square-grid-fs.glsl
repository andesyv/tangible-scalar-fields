#version 450
#include "/defines.glsl"

layout (location = 0) out vec4 squareGridTexture;

uniform vec3 squareBorderColor;

void main()
{
	squareGridTexture = vec4(squareBorderColor, 1.0f);
}