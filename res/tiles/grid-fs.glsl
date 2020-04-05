#version 450
#include "/defines.glsl"
#include "/globals.glsl"

layout (location = 0) out vec4 gridTexture;

uniform vec3 borderColor;

void main()
{
	gridTexture = vec4(borderColor, 1.0f);
}