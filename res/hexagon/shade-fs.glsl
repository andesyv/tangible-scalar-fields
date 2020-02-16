#version 450
#include "/defines.glsl"
#include "/globals.glsl"

layout(pixel_center_integer) in vec4 gl_FragCoord;
layout (location = 0) out vec4 colorTexture;

uniform sampler2D pointChartTexture;

void main()
{

    vec4 pointCol = texelFetch(pointChartTexture, ivec2(gl_FragCoord.xy), 0).rgba;

    colorTexture = pointCol;
}


