#version 450
#include "/defines.glsl"
#include "/globals.glsl"

layout(pixel_center_integer) in vec4 gl_FragCoord;
layout (location = 0) out vec4 pointCircleTexture;
layout (location = 1) out vec4 densityEstimation;

const float PI = 3.14159265359;

uniform float sigma2;			
uniform float radius;
uniform float radiusMult;
uniform float densityMult;

in vec2 isCoords;

uniform vec3 pointColor;

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

	float blendRadius = smoothstep(0.5, 1.0, fragDistanceFromCenter);

    pointCircleTexture = vec4(pointColor, 1.0f-blendRadius);
	//OpenGL expects premultiplied alpha values
	pointCircleTexture.rgb *= pointCircleTexture.a;

	//TODO set correct define
	#if defined(RENDER_KDE) || defined(RENDER_DENSITY_NORMALS) || defined(RENDER_TILE_NORMALS)
		densityEstimation = vec4(gaussKernel(fragDistanceFromCenter*radius*radiusMult)*densityMult, 0.0f, 0.0f, 1.0f);
	#endif
}