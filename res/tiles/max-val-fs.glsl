#version 450
#include "/defines.glsl"
#include "/globals.glsl"

//--in
layout(pixel_center_integer) in vec4 gl_FragCoord;

layout(std430, binding = 2) buffer valueMaxBuffer
{
	uint maxAccumulate;
    uint maxPointAlpha;
};

//--in
layout(pixel_center_integer) in vec4 gl_FragCoord;

in vec4 boundsScreenSpace;

uniform sampler2D accumulateTexture;
uniform sampler2D pointCircleTexture;

//min = 0
uniform int maxTexCoordX;
uniform int maxTexCoordY;

void main()
{
    
    // to get intervals from 0 to maxTexCoord, we map the original Point interval to maxTexCoord+1
    // If the current index = maxIndex+1, we take the maxTexCoord instead
    int squareX = min(maxTexCoordX, mapInterval(gl_FragCoord.x, boundsScreenSpace[2], boundsScreenSpace[0], maxTexCoordX+1));
    int squareY = min(maxTexCoordY, mapInterval(gl_FragCoord.y, boundsScreenSpace[3], boundsScreenSpace[1], maxTexCoordY+1));

    // get value from accumulate texture
    float squareAccValue = texelFetch(accumulateTexture, ivec2(squareX,squareY), 0).r;

    //// floatBitsToUint: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/floatBitsToInt.xhtml
    uint uintSquareAccValue = floatBitsToUint(squareAccValue);

    // identify max value
    atomicMax(maxAccumulate, uintSquareAccValue);
    
    float pointAlpha = texelFetch(pointCircleTexture, ivec2(gl_FragCoord.x, gl_FragCoord.y),0).a;

	uint uintPointAlphaValue = floatBitsToUint(pointAlpha);

	atomicMax(maxPointAlpha, uintPointAlphaValue);
}
