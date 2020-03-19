#version 450
#include "/defines.glsl"
#include "/globals.glsl"

layout(pixel_center_integer) in vec4 gl_FragCoord;
layout (location = 0) out vec4 colorTexture;

layout(std430, binding = 6) buffer valueMaxBuffer
{
    uint maxAccumulate;
    uint maxPointAlpha;
};


uniform sampler2D pointChartTexture;
uniform sampler2D pointCircleTexture;
uniform sampler2D tilesTexture;
uniform sampler2D gridTexture;
uniform sampler2D accPointTexture;

void main()
{

    vec4 col = texelFetch(pointChartTexture, ivec2(gl_FragCoord.xy), 0).rgba;
    float alpha;

    vec4 tilesCol;
    vec4 poinCircleCol;
    bool blendPointCircles=false;

    #ifdef RENDER_POINT_CIRCLES

        // read from SSBO and convert back to float: --------------------------------------------------------------------
        // - https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/intBitsToFloat.xhtml
        float floatMaxPointAlpha = uintBitsToFloat(maxPointAlpha);

        poinCircleCol = texelFetch(pointCircleTexture, ivec2(gl_FragCoord.xy), 0).rgba;
        //TODO: use alpha/maxAlpha
        poinCircleCol /= floatMaxPointAlpha;
        //poinCircleCol.a = poinCircleCol.a/floatMaxPointAlpha;
        //poinCircleCol.a = max(1, poinCircleCol.a);
        blendPointCircles = true;

        // debug only
        col = poinCircleCol;
    #endif 

    #ifdef RENDER_SQUARES
        tilesCol = texelFetch(tilesTexture, ivec2(gl_FragCoord.xy), 0).rgba;

        //TODO: maybe make #define
        if(blendPointCircles)
        {
            if(tilesCol.r > 0 || tilesCol.g > 0 || tilesCol.b > 0){

                float alphaBlend = tilesCol.a + (1-tilesCol.a) * poinCircleCol.a;
                col = 1/alphaBlend * (tilesCol.a*tilesCol + (1-tilesCol.a)*poinCircleCol.a*poinCircleCol);
            }
        }
        else
        {
            col = tilesCol;
        }
    #endif

    #ifdef RENDER_SQUARE_GRID
        vec4 gridCol = texelFetch(gridTexture, ivec2(gl_FragCoord.xy), 0).rgba;
        if(gridCol.r > 0 || gridCol.g > 0 || gridCol.b > 0){
            col = gridCol;
        }
    #endif

    #ifdef RENDER_ACC_POINTS
        col = max(col, texelFetch(accPointTexture, ivec2(gl_FragCoord.xy), 0).rgba);
    #endif

    colorTexture = col;
}


