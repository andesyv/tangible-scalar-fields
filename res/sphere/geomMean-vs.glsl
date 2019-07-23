#version 450
#extension GL_ARB_shading_language_include : require
#include "/defines.glsl"

layout(location = 0) in float xValue;
layout(location = 1) in float yValue;
layout(location = 2) in float radiusValue;
layout(location = 3) in float colorValue;

uniform mat4 modelViewProjectionMatrix;

void main()
{
	vec4 vertexPosition = vec4(xValue, yValue, 0.0f, 1.0f);

	gl_Position = modelViewProjectionMatrix * vertexPosition;
}