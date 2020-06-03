#version 450
#include "/defines.glsl"
#include "/globals.glsl"

#define N 4

layout(points) in;
layout(triangle_strip, max_vertices = N+1) out;

uniform sampler2D accumulateTexture;

//[x,y]
uniform vec2 maxBounds_Off;
uniform vec2 minBounds_Off;

uniform int windowHeight;
uniform int windowWidth;

uniform float tileSize;
uniform mat4 modelViewProjectionMatrix;

uniform int numCols;
uniform int numRows;

const float PI = 3.1415926;

out vec2 isCoords;
out float tileSizeScreenSpace;
out vec2 tileCenterScreenSpace;

void main()
{

    // get value from accumulate texture
    float squareValue = texelFetch(accumulateTexture, ivec2(gl_in[0].gl_Position.x,gl_in[0].gl_Position.y), 0).r;

    // we dont want to render the grid for empty squares
    if(squareValue > 0){

        // map from square space to bounding box space
        float bbX = mapInterval_O(gl_in[0].gl_Position.x, 0, numCols, minBounds_Off[0], maxBounds_Off[0]);
        float bbY = mapInterval_O(gl_in[0].gl_Position.y, 0, numRows, minBounds_Off[1], maxBounds_Off[1]);

        vec4 bbPos = vec4(bbX, bbY, 0.0f, 1.0f);

        tileSizeScreenSpace = getScreenSpaceSize(modelViewProjectionMatrix, vec2(tileSize, 0.0f), windowWidth, windowHeight).x;
        tileCenterScreenSpace = getScreenSpacePosOfPoint(modelViewProjectionMatrix, vec2(bbPos) + tileSize/2.0f, windowWidth, windowHeight);

        //Emit 5 vertices for square
        vec4 offset;
        vec4 pos;

        //bottom left
        pos = modelViewProjectionMatrix * (bbPos + vec4(0.0,0.0,0.0,0.0));
        isCoords = vec2(-1, -1);
        gl_Position = pos;
        EmitVertex();
        //bottom right
        pos = modelViewProjectionMatrix * (bbPos + vec4(0, tileSize,0.0,0.0));
        isCoords = vec2(-1, 1);
        gl_Position = pos;
        EmitVertex();
        //top left
        pos = modelViewProjectionMatrix * (bbPos + vec4(tileSize, 0,0.0,0.0));
        isCoords = vec2(1, -1);
        gl_Position = pos;
        EmitVertex();
        //top right
        pos = modelViewProjectionMatrix * (bbPos + vec4(tileSize, tileSize,0.0,0.0));
        isCoords = vec2(1, 1);
        gl_Position = pos;
        EmitVertex();

        /*pos = modelViewProjectionMatrix * (bbPos + vec4(0.0,0.0,0.0,0.0));
        gl_Position = pos;
        EmitVertex();*/

        EndPrimitive();
    }
}