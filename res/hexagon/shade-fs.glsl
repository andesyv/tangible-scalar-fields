#version 450
#include "/defines.glsl"
#include "/globals.glsl"

layout(pixel_center_integer) in vec4 gl_FragCoord;
layout (location = 0) out vec4 colorTexture;

uniform sampler2D pointChartTexture;
uniform sampler2D pointCircleTexture;
uniform sampler2D tilesTexture;
uniform sampler2D gridTexture;
uniform sampler2D accPointTexture;

void main()
{

    vec4 col = texelFetch(pointChartTexture, ivec2(gl_FragCoord.xy), 0).rgba;
    float alpha;
    bool blendPointCircles=false;

    #ifdef RENDER_POINT_CIRCLES
        col = texelFetch(pointCircleTexture, ivec2(gl_FragCoord.xy), 0).rgba;
        alpha = col.a;
        blendPointCircles = true;
    #endif 

    #ifdef RENDER_SQUARES
        vec4 tilesCol = texelFetch(tilesTexture, ivec2(gl_FragCoord.xy), 0).rgba;

        //TODO: maybe make #define
        if(blendPointCircles)
        {
            col = col + (tilesCol * (1-alpha));
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


