#version 450
#include "/defines.glsl"

#define N 4

layout(points) in;
layout(line_strip, max_vertices = N+1) out;

uniform float squareSize;
uniform mat4 modelViewProjectionMatrix;

const float PI = 3.1415926;

void main()
{

		//Emit 5 vertices for square
		vec4 offset;
        vec4 pos;

        pos = modelViewProjectionMatrix * (gl_in[0].gl_Position + vec4(0.0,0.0,0.0,0.0));
        gl_Position = pos;
        EmitVertex();

        pos = modelViewProjectionMatrix * (gl_in[0].gl_Position + vec4(0, squareSize,0.0,0.0));
        gl_Position = pos;
        EmitVertex();

        pos = modelViewProjectionMatrix * (gl_in[0].gl_Position + vec4(squareSize, squareSize,0.0,0.0));
        gl_Position = pos;
        EmitVertex();

        pos = modelViewProjectionMatrix * (gl_in[0].gl_Position + vec4(squareSize, 0,0.0,0.0));
        gl_Position = pos;
        EmitVertex();

        pos = modelViewProjectionMatrix * (gl_in[0].gl_Position + vec4(0.0,0.0,0.0,0.0));
        gl_Position = pos;
        EmitVertex();

		EndPrimitive();
}