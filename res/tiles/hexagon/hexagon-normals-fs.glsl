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


//some pixels of rect row 0 do nto fall inside a hexagon.
//there we need to discard them
bool discardOutsideOfGridFragments(vec2 minBounds, vec2 maxBounds){
    // to get intervals from 0 to maxCoord, we map the original Point interval to maxCoord+1
    // If the current value = maxValue, we take the maxCoord instead
    int rectX = min(max_rect_col, mapInterval(gl_FragCoord.x, minBounds.x, maxBounds.x, max_rect_col + 1));
    int rectY = min(max_rect_row, mapInterval(gl_FragCoord.y, minBounds.y, maxBounds.y, max_rect_row + 1));

    if (rectY == 0 && mod(rectX, 3) != 2)
    {
        // rectangle left lower corner in space of points
        vec2 ll = vec2(rectX * rectSizeScreenSpace.x + minBounds.x, minBounds.y);
        vec2 a, b;
        //modX = 0
        if (mod(rectX, 3) == 0)
        {
            //Upper Left
            a = ll;
            b = vec2(ll.x + rectSizeScreenSpace.x / 2.0f, ll.y + rectSizeScreenSpace.y);
            if (pointLeftOfLine(a, b, vec2(gl_FragCoord)))
            {
                return true;
            }
        }
        // modX = 1
        else
        {
            //Upper Right
            a = vec2(ll.x + rectSizeScreenSpace.x / 2.0f, ll.y + rectSizeScreenSpace.y);
            b = vec2(ll.x + rectSizeScreenSpace.x, ll.y);
            if (pointLeftOfLine(a, b, vec2(gl_FragCoord)))
            {
                return true;
            }
        }
    }
    return false;
}

void main()
{
    vec2 minBounds = vec2(boundsScreenSpace[2], boundsScreenSpace[3]);
    vec2 maxBounds = vec2(boundsScreenSpace[0], boundsScreenSpace[1]);

    if(discardOutsideOfGridFragments(minBounds, maxBounds)){
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
    
    // get value from accumulate texture
    vec4 fragmentNormal = vec4(calculateNormalFromHeightMap(ivec2(gl_FragCoord.xy), kdeTexture), 1.0f);
    //vec4 fragmentNormal = texelFetch(densityNormalsTexture, ivec2(gl_FragCoord.xy), 0);

    for(int i = 0; i < 4; i++){
        uint uintValue = floatBitsToUint(fragmentNormal[i]);
        atomicAdd(tileNormals[int((hex.x * maxTexCoordY + hex.y) * 4 + i)], uintValue);
    }
}
