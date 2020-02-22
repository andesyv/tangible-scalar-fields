#version 450
#extension GL_ARB_shading_language_include : require
#include "/defines.glsl"

layout(location = 0) in float xValue;
layout(location = 1) in float yValue;

uniform mat4 modelViewProjectionMatrix;

// 1D color map parameters
uniform sampler1D colorMapTexture;
uniform int textureWidth;
uniform float viewportX;

//[x,y]
uniform vec2 maxBounds_Off;
uniform vec2 minBounds;
//min = 0
uniform int maxTexCoordX;
uniform int maxTexCoordY;

out vec4 vertexColor;

//maps value x from [a,b] --> [0,c]
int mapInterval(float x, float a, float b, int c){
    return int((x-a)*c/(b-a));
}

void main()
{

    // to get intervals from 0 to maxTexCoord, we map the original Point interval to maxTexCoord+1
    // If the current value = maxValue, we take the maxTexCoord instead
    int squareX = min(maxTexCoordX, mapInterval(xValue, minBounds[0], maxBounds_Off[0], maxTexCoordX+1));
    int squareY = min(maxTexCoordY, mapInterval(yValue, minBounds[1], maxBounds_Off[1], maxTexCoordY+1));

    // we only set the red channel, because we only use the color for additive blending
    vertexColor = vec4(1.0f,0.0f,0.0f,1.0f);

    // debug: color point according to square
    //vertexColor = vec4(float(squareX/float(maxTexCoord)),float(squareY/float(maxTexCoord)),0.0f,1.0f);

    // NDC - map square coordinates to [-1,1] (first [0,2] than -1)
	vec2 squareNDC = vec2(((squareX * 2) / float(maxTexCoordX)) - 1, ((squareY * 2) / float(maxTexCoordY)) - 1);

    gl_Position = vec4(squareNDC, 0.0f, 1.0f);
}