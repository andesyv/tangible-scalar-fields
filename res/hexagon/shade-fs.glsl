#version 450
#include "/defines.glsl"
#include "/globals.glsl"

layout(pixel_center_integer) in vec4 gl_FragCoord;
in vec4 gFragmentPosition;
out vec4 fragColor;

// texture for blending surface and classical line chart
uniform sampler2D pointChartTexture;

void main()
{
    fragColor = texelFetch(pointChartTexture, ivec2(gl_FragCoord.xy), 0).rgba;
}


