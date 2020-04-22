#version 450
#include "/defines.glsl"

layout(pixel_center_integer) in vec4 gl_FragCoord;
const float PI = 3.14159265359;

uniform float sigma2;			

layout (location = 0) out vec4 densityEstimation;

in vec2 isCoords;

float gaussKernel(float d)
{
    return 1 / (sigma2 * sqrt(2 * PI)) * exp(-(pow(d, 2) / (2 * sigma2)));
}

void main()
{
    float fragDistanceFromCenter = length(isCoords);

	// create circle
	if (fragDistanceFromCenter > 1) {
		discard;
	}

	//TODO do not use imposter coordinates
    densityEstimation = vec4(gaussKernel(fragDistanceFromCenter), 0.0f, 0.0f, 1.0f);
}