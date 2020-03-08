#version 450
#include "/defines.glsl"
#include "/globals.glsl"

#define N 4

layout(points) in;
layout(line_strip, max_vertices = N+1) out;

uniform sampler2D squareAccumulateTexture;

//[x,y]
uniform vec2 maxBounds_Off;
uniform vec2 minBounds;

uniform float squareSize;
uniform mat4 modelViewProjectionMatrix;

uniform int numCols;
uniform int numRows;

const float PI = 3.1415926;

void main()
{

    // get value from accumulate texture
    float squareValue = texelFetch(squareAccumulateTexture, ivec2(gl_in[0].gl_Position.x,gl_in[0].gl_Position.y), 0).r;

    // we dont want to render the grid for empty squares
    if(squareValue > 0.00000001){

        // map from square space to bounding box space
        float bbX = mapInterval_O(gl_in[0].gl_Position.x, 0, numCols, minBounds[0], maxBounds_Off[0]);
        float bbY = mapInterval_O(gl_in[0].gl_Position.y, 0, numRows, minBounds[1], maxBounds_Off[1]);

        vec4 bbPos = vec4(bbX, bbY, 0.0f, 1.0f);

        //Emit 5 vertices for square
        vec4 offset;
        vec4 pos;

        pos = modelViewProjectionMatrix * (bbPos + vec4(0.0,0.0,0.0,0.0));
        gl_Position = pos;
        EmitVertex();

        pos = modelViewProjectionMatrix * (bbPos + vec4(0, squareSize,0.0,0.0));
        gl_Position = pos;
        EmitVertex();

        pos = modelViewProjectionMatrix * (bbPos + vec4(squareSize, squareSize,0.0,0.0));
        gl_Position = pos;
        EmitVertex();

        pos = modelViewProjectionMatrix * (bbPos + vec4(squareSize, 0,0.0,0.0));
        gl_Position = pos;
        EmitVertex();

        pos = modelViewProjectionMatrix * (bbPos + vec4(0.0,0.0,0.0,0.0));
        gl_Position = pos;
        EmitVertex();

        EndPrimitive();
    }
}