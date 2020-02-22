#version 450
#include "/defines.glsl"
#include "/globals.glsl"

//--in
layout(pixel_center_integer) in vec4 gl_FragCoord;

layout(std430, binding = 2) buffer valueMaxBuffer
{
	uint maxValue;
};

//--in
layout(pixel_center_integer) in vec4 gl_FragCoord;

in vec4 boundsScreenSpace;

uniform sampler2D squareAccumulateTexture;

//min = 0
uniform int maxTexCoordX;
uniform int maxTexCoordY;

//maps value x from [a,b] --> [0,c]
int mapInterval(float x, float a, float b, int c){
    return int((x-a)*c/(b-a));
}

void main()
{
    
    // to get intervals from 0 to maxTexCoord, we map the original Point interval to maxTexCoord+1
    // If the current index = maxIndex+1, we take the maxTexCoord instead
    int squareX = min(maxTexCoordX, mapInterval(gl_FragCoord.x, boundsScreenSpace[2], boundsScreenSpace[0], maxTexCoordX+1));
    int squareY = min(maxTexCoordY, mapInterval(gl_FragCoord.y, boundsScreenSpace[3], boundsScreenSpace[1], maxTexCoordY+1));

    // get value from accumulate texture
    float squareValue = texelFetch(squareAccumulateTexture, ivec2(squareX,squareY), 0).r;

    //// floatBitsToUint: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/floatBitsToInt.xhtml
	uint uintValue = floatBitsToUint(squareValue);

	// identify max value
	atomicMax(maxValue, uintValue);
}
