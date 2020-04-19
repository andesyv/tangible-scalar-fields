#version 450
#include "/defines.glsl"

const float PI = 3.14159265359;

// Normal distribution N(mue,sigma^2) 
uniform float sigma2;			
uniform float gaussScale;
uniform vec2 windowSize;

uniform sampler2D pointChartTexture;

layout (location = 0) out vec4 densityEstimation;

float gaussKernel(float x, float y,float sigma)
{
	return 1 / (sigma * sqrt(2 * PI)) *
           exp(-(pow(x, 2) / (2 * sigma) + pow(y, 2) / (2 * sigma, 2)));
}

void main()
{
    float density = gaussKernel(gl_FragCoord.x/windowSize.x, gl_FragCoord.y/windowSize.y, sigma2);

    vec4 pointCol = texelFetch(pointChartTexture, ivec2(gl_FragCoord.xy), 0).rgba;

    densityEstimation = pointCol;
    //densityEstimation = vec4(density, 0.0f, 0.0f, 1.0f);
}