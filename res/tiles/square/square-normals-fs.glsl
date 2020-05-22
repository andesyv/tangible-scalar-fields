#version 450
#include "/defines.glsl"
#include "/globals.glsl"

//--in
layout(pixel_center_integer) in vec4 gl_FragCoord;

layout(std430, binding = 0) buffer tileNormalsBuffer
{
    int tileNormals[];
};

in vec4 boundsScreenSpace;
in float tileSizeScreenSpace;

uniform sampler2D accumulateTexture;
uniform sampler2D kdeTexture;

//min = 0
uniform int maxTexCoordX;
uniform int maxTexCoordY;

uniform float normalsFactor;

void main()
{
    
    // to get intervals from 0 to maxTexCoord, we map the original Point interval to maxTexCoord+1
    // If the current index = maxIndex+1, we take the maxTexCoord instead
    int squareX = min(maxTexCoordX, mapInterval(gl_FragCoord.x, boundsScreenSpace[2], boundsScreenSpace[0], maxTexCoordX+1));
    int squareY = min(maxTexCoordY, mapInterval(gl_FragCoord.y, boundsScreenSpace[3], boundsScreenSpace[1], maxTexCoordY+1));

    // we don't want to render fragments outside the grid
    if(squareX < 0 || squareY < 0){
        discard;
    }
    // get value from accumulate texture
    float squareValue = texelFetch(accumulateTexture, ivec2(squareX,squareY), 0).r;
    // we don't want to render empty squares
    if(squareValue < 0.00000001){
        discard;
    }

    // get value from density normals texture
    vec4 fragmentNormal = vec4(calculateNormalFromHeightMap(ivec2(gl_FragCoord.xy), kdeTexture), 1.0f);
    fragmentNormal *= normalsFactor;
   
    for(int i = 0; i < 4; i++){
        int intValue = int(fragmentNormal[i]);

        atomicAdd(tileNormals[(squareX*(maxTexCoordY+1) + squareY) * 5 + i], intValue);
    }

    //TODO: show thomas
    //accumulate height of kdeTexture
    //divide with tileSize^2 to achieve approximate scale independency
    //small scale tiles will in general have higher height, because the height does not grow with power of 2
    float kdeHeight = texelFetch(kdeTexture, ivec2(gl_FragCoord.xy), 0).r / pow(tileSizeScreenSpace,2);
    kdeHeight *= normalsFactor;
    atomicAdd(tileNormals[(squareX*(maxTexCoordY+1) + squareY) * 5 + 4], int(kdeHeight));
}
