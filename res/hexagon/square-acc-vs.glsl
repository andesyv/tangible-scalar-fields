#version 450
#extension GL_ARB_shading_language_include : require
#include "/defines.glsl"

layout(location = 0) in float xValue;
layout(location = 1) in float yValue;

// 1D color map parameters
uniform sampler1D colorMapTexture;
uniform int textureWidth;
uniform float viewportX;

//[x,y]
uniform vec2 maxBounds;
uniform vec2 minBounds;
//for now we assume texture is square maxX=maxY
//min = 0
uniform int maxTexCoord;

out vec4 vertexColor;

//maps value x from [a,b] --> [0,c]
int mapInterval(float x, float a, float b, int c){
    return int((x-a)*c/(b-a));
}

void main()
{

    // to get intervals from 0 to maxTexCoord, we map the original Point interval to maxTexCoord+1
    // If the current value = maxValue, we take the maxTexCoord instead
    int squareX = min(maxTexCoord, mapInterval(xValue, minBounds[0], maxBounds[0], maxTexCoord+1));
    int squareY = min(maxTexCoord, mapInterval(yValue, minBounds[1], maxBounds[1], maxTexCoord+1));

	//int colorTexelCoord = int(((squareX+squareY) * textureWidth)/(2*maxTexCoord+1));
	//vertexColor = vec4(texelFetch(colorMapTexture, colorTexelCoord, 0).rgb, 1.0f);

    vertexColor = vec4(1.0f,1.0f,1.0f,1.0f);

	gl_Position = vec4(squareX, squareY, 0.0f, 1.0f);
}