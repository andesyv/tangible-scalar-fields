#version 450
#include "/defines.glsl"
#include "/globals.glsl"
#include "/hex/globals.glsl"

//--in
layout(pixel_center_integer) in vec4 gl_FragCoord;

layout(std430, binding = 0) buffer tileNormalsBuffer
{
    uint tileNormals[];
};

in vec4 boundsScreenSpace;
in vec2 rectSizeScreenSpace;

uniform sampler2D accumulateTexture;
uniform sampler2D densityNormalsTexture;
uniform sampler2D kdeTexture;

//min = 0
uniform int maxTexCoordX;
uniform int maxTexCoordY;

uniform int max_rect_col;
uniform int max_rect_row;

uniform float normalsFactor;

//--out
layout(location = 0) out vec4 hexTilesTexture;

void main()
{
    vec2 minBounds = vec2(boundsScreenSpace[2], boundsScreenSpace[3]);
    vec2 maxBounds = vec2(boundsScreenSpace[0], boundsScreenSpace[1]);

    if(discardOutsideOfGridFragments(vec2(gl_FragCoord), minBounds, maxBounds, rectSizeScreenSpace, max_rect_col, max_rect_row)){
        discard;
    }

    vec2 hex = matchPointWithHexagon(vec2(gl_FragCoord), max_rect_col, max_rect_row, rectSizeScreenSpace.x, rectSizeScreenSpace.y,
                                     minBounds, maxBounds);

    // we don't want to render fragments outside the grid
    if(hex.x < 0 || hex.y < 0){
        discard;
    }
    // get value from accumulate texture
    float hexValue = texelFetch(accumulateTexture, ivec2(hex.x, hex.y), 0).r;
    // we don't want to render empty hexs
    if (hexValue < 0.00000001)
    {
        discard;
    }
    
   // get value from density normals texture
    vec4 fragmentNormal = vec4(calculateNormalFromHeightMap(ivec2(gl_FragCoord.xy), kdeTexture), 1.0f);
    fragmentNormal *= normalsFactor;
   
    for(int i = 0; i < 4; i++){
        uint uintValue = uint(fragmentNormal[i]);

        atomicAdd(tileNormals[int((hex.x*(maxTexCoordY+1) + hex.y) * 4 + i)], uintValue);
    }
}
