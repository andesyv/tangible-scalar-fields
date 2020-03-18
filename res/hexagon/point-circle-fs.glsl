#version 450
#include "/defines.glsl"
#include "/globals.glsl"

layout(pixel_center_integer) in vec4 gl_FragCoord;
layout (location = 0) out vec4 pointCircleTexture;

in vec2 isCoords;

uniform vec3 pointColor;

void main()
{

    float fragPosInCircle = length(isCoords);

	// create circle
	if (fragPosInCircle > 1) {
		discard;
	}

	float blendRadius = smoothstep(0.5, 1.0, fragPosInCircle);

    pointCircleTexture = vec4(pointColor, 1.0) * (1.0f-blendRadius);
	//OpenGL expects premultiplied alpha values
	pointCircleTexture.rgb *= pointCircleTexture.a;

}