#version 450
#extension GL_ARB_shading_language_include : require
#include "/defines.glsl"

layout(location = 0) in float xValue;

uniform float horizontal_space;
uniform float vertical_space;


void main()
{
	gl_Position = vec4(gl_VertexID*5, gl_VertexID*5, 0.0f, 1.0f);
}