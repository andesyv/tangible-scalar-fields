#version 450
#include "/defines.glsl"

layout(pixel_center_integer) in vec4 gl_FragCoord;

// estimated density from previous render pass
uniform sampler2D pilotDensityTexture;

layout(std430, binding = 1) buffer geomMeanBuffer
{
	int geomMean;
};

void main()
{
	float samplePointDensity = log(texelFetch(pilotDensityTexture,ivec2(gl_FragCoord.xy),0).r);

	// Fixpoint arithmetic: increase precision an convert to uint
	atomicAdd(geomMean, int(samplePointDensity * 256));
}