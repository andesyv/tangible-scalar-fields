#version 450
#extension GL_ARB_shading_language_include : require
#include "/defines.glsl"

layout(location = 0) in float xValue;
layout(location = 1) in float yValue;
layout(location = 2) in float radiusValue;
layout(location = 3) in float colorValue;

out VertexData
{
	float radiusValueVS;
	float colorValueVS;
} outData;

uniform float radiusMultiplier;


void main()
{
	vec4 vertexPosition = vec4(xValue, yValue, 0.0f, 1.0f);

	// allow the user to scale the sphere-radius using a GUI-slider ('m_radiusMultiplier')
	outData.radiusValueVS = radiusValue*radiusMultiplier;
	outData.colorValueVS = colorValue;

	gl_Position = vertexPosition;
}