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


in vec4 boundsScreenSpace;
in vec2 rectSizeScreenSpace;

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

//--out
layout (location = 0) out vec4 hexTilesTexture;

void main()
{

    vec2 hex = matchPointWithHexagon(vec2(gl_FragCoord), max_rect_col, max_rect_row, rectSizeScreenSpace.x, rectSizeScreenSpace.y, 
                                    vec2(boundsScreenSpace[2],boundsScreenSpace[3]), vec2(boundsScreenSpace[0], boundsScreenSpace[1]));

     // get value from accumulate texture
    float hexValue = texelFetch(accumulateTexture, ivec2(hex.x,hex.y), 0).r;

    // we don't want to render empty hexs
    if(hexValue < 0.00000001){
        discard;
    }

	// read from SSBO and convert back to float: --------------------------------------------------------------------
	// - https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/intBitsToFloat.xhtml
    // +1 because else we cannot map the maximum value itself
    float floatMaxAccumulate = uintBitsToFloat(maxAccumulate) + 1;

    //color hexs according to index
    hexTilesTexture = vec4(float(hex.x/float(maxTexCoordX)),float(hex.y/float(maxTexCoordY)),0.0f,1.0f);

    // color hex according to value
	#ifdef COLORMAP
		int colorTexelCoord = mapInterval(hexValue, 0, floatMaxAccumulate, textureWidth);
		hexTilesTexture.rgb = texelFetch(colorMapTexture, colorTexelCoord, 0).rgb;
	#endif

    #ifdef RENDER_DISCREPANCY
        //discrepancy is set as opacity
        float tilesDiscrepancy = texelFetch(tilesDiscrepancyTexture, ivec2(hex.x,hex.y), 0).r;
        hexTilesTexture *= tilesDiscrepancy;
    #endif
}
