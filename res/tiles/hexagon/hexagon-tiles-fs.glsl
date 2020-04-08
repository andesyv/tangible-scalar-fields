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

uniform float rectHeight;
uniform float rectWidth;

//--out
layout (location = 0) out vec4 hexTilesTexture;

void main()
{

    vec2 hex = matchPointWithHexagon(vec2 (gl_FragCoord), max_rect_col, max_rect_row, rectWidth, rectHeight, 
    vec2(boundsScreenSpace[2],boundsScreenSpace[3]), vec2(boundsScreenSpace[0], boundsScreenSpace[]));

/*
    // to get intervals from 0 to maxCoord, we map the original Point interval to maxCoord+1
    // If the current value = maxValue, we take the maxCoord instead
    int rectX = min(max_rect_col, mapInterval(gl_FragCoord.x, boundsScreenSpace[2], boundsScreenSpace[0], max_rect_col+1));
    int rectY = min(max_rect_row, mapInterval(gl_FragCoord.y, boundsScreenSpace[3], boundsScreenSpace[1], max_rect_row+1));

    // rectangle left lower corner in space of points
    vec2 ll = vec2(rectX * rectWidth + boundsScreenSpace[2] , rectY * rectHeight + boundsScreenSpace[3]);
    vec2 p = vec2(xValue, yValue);
    vec2 a, b;

    // calculate hexagon index from rectangle index
    int hexX, hexY, modX, modY;
    
    // get modulo values
    modX = int(mod(rectX,3));
    modY = int(mod(rectY,2));

    hexY = int(rectY/2);

    //calculate X index
    hexX = int(rectX/3) * 2 + modX;
    if(modX != 0){
        if(modX == 1){
            if(modY == 0){
                //Upper Left
                a = ll;
                b = vec2(ll.x + rectWidth/2.0f, ll.y + rectHeight);
                if(pointOutsideHex(a,b,p)){
                    hexX--;
                }
            }
            //modY = 1
            else{
                //Lower Left
                a = vec2(ll.x + rectWidth/2.0f, ll.y);
                b = vec2(ll.x, ll.y + rectHeight);
                if(pointOutsideHex(a,b,p)){
                    hexX--;
                }
            }
        }
        //modX = 2
        else{

            if(modY == 0){
                //Upper Right
                a = vec2(ll.x + rectWidth, ll.y);
                b = vec2(ll.x + rectWidth/2.0f, ll.y + rectHeight);
                if(pointOutsideHex(a,b,p)){
                    hexX++;
                }
            }
            //modY = 1
            else{
                //Lower Right
                a = vec2(ll.x + rectWidth/2.0f, ll.y);
                b = vec2(ll.x + rectWidth, ll.y + rectHeight);
                if(pointOutsideHex(a,b,p)){
                    hexX++;
                }
            }
        }
    }
    */
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
