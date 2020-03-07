#version 450
#include "/defines.glsl"
#include "/globals.glsl"

layout(pixel_center_integer) in vec4 gl_FragCoord;
layout (location = 0) out vec4 colorTexture;

uniform sampler2D pointChartTexture;
uniform sampler2D tilesTexture;
uniform sampler2D gridTexture;
uniform sampler2D accPointTexture;

void main()
{

    vec4 col = texelFetch(pointChartTexture, ivec2(gl_FragCoord.xy), 0).rgba;

    #ifdef RENDER_SQUARES
        col = texelFetch(gridTexture, ivec2(gl_FragCoord.xy), 0).rgba;
        col = max(col, texelFetch(tilesTexture, ivec2(gl_FragCoord.xy), 0).rgba);
    #endif

    #ifdef RENDER_ACC_POINTS
        col = max(col, texelFetch(accPointTexture, ivec2(gl_FragCoord.xy), 0).rgba);
    #endif

    colorTexture = col;
}


