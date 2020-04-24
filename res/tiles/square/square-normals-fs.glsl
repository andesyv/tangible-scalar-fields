#version 450
#include "/defines.glsl"
#include "/globals.glsl"

//--in
layout(pixel_center_integer) in vec4 gl_FragCoord;

layout(std430, binding = 2) buffer valueMaxBuffer
{
    uint tileNormals[];
};

in vec4 boundsScreenSpace;

uniform sampler2D densityNormalsTexture;

//min = 0
uniform int maxTexCoordX;
uniform int maxTexCoordY;

void main()
{
    
    // to get intervals from 0 to maxTexCoord, we map the original Point interval to maxTexCoord+1
    // If the current index = maxIndex+1, we take the maxTexCoord instead
    int squareX = min(maxTexCoordX, mapInterval(gl_FragCoord.x, boundsScreenSpace[2], boundsScreenSpace[0], maxTexCoordX+1));
    int squareY = min(maxTexCoordY, mapInterval(gl_FragCoord.y, boundsScreenSpace[3], boundsScreenSpace[1], maxTexCoordY+1));

    //TODO: test if necessary
    // we don't want to render fragments outside the grid
    if(squareX < 0 || squareY < 0){
        discard;
    }

    //TODO: maybe discard empty tiles for performance

    // get value from density normals texture
    vec4 fragmentNormal = texelFetch(densityNormalsTexture, ivec2(gl_FragCoord.xy), 0);

    for(int i = 0; i < 4; i++){
        uint uintValue = floatBitsToUint(fragmentNormal[i]);
        atomicAdd(tileNormals[((squareX*maxTexCoordY) + squareY) * 4 + i], uintValue);
    }
}
