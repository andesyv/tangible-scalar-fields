#version 450
#include "/defines.glsl"
#include "/globals.glsl"

//--in
layout(pixel_center_integer) in vec4 gl_FragCoord;

layout(std430, binding = 1) buffer valueMaxBuffer
{
    uint maxAccumulate;
    uint maxPointAlpha;
};

in vec4 boundsScreenSpace;
in vec2 rectSizeScreenSpace;
in float tileSizeScreenSpace;

uniform sampler2D accumulateTexture;
uniform sampler2D tilesDiscrepancyTexture;

// 1D color map parameters
uniform sampler1D colorMapTexture;
uniform int textureWidth;

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
            if (pointInsideHex(a, b, vec2(gl_FragCoord)))
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
            if (pointInsideHex(a, b, vec2(gl_FragCoord)))
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

    // get value from accumulate texture
    float hexValue = texelFetch(accumulateTexture, ivec2(hex.x, hex.y), 0).r;

    // we don't want to render empty hexs
    if (hexValue < 0.00000001)
    {
        discard;
    }

    // read from SSBO and convert back to float: --------------------------------------------------------------------
    // - https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/intBitsToFloat.xhtml
    // +1 because else we cannot map the maximum value itself
    float floatMaxAccumulate = uintBitsToFloat(maxAccumulate) + 1;

    //color hexs according to index
    hexTilesTexture = vec4(float(hex.x / float(maxTexCoordX)), float(hex.y / float(maxTexCoordY)), 0.0f, 1.0f);

// color hex according to value
#ifdef COLORMAP
    int colorTexelCoord = mapInterval(hexValue, 0, floatMaxAccumulate, textureWidth);
    hexTilesTexture.rgb = texelFetch(colorMapTexture, colorTexelCoord, 0).rgb;
#endif

#ifdef RENDER_DISCREPANCY
    //discrepancy is set as opacity
    float tilesDiscrepancy = texelFetch(tilesDiscrepancyTexture, ivec2(hex.x, hex.y), 0).r;
    hexTilesTexture *= tilesDiscrepancy;
#endif
}
