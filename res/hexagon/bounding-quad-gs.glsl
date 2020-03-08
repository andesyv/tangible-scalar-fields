#version 450
#include "/defines.glsl"
#include "/globals.glsl"

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

//[x,y]
uniform vec2 maxBounds_Off;
uniform vec2 maxBounds;
uniform vec2 minBounds;

uniform int windowHeight;
uniform int windowWidth;

uniform mat4 modelViewProjectionMatrix;

out vec4 boundsScreenSpace;
out vec2 origMaxBoundScreenSpace;

void main()
{

    boundsScreenSpace = getScreenSpacePosOfRect(modelViewProjectionMatrix, maxBounds_Off, minBounds, windowWidth, windowHeight);

	// we also convert the original bounding box maximum bounds
	// this way we can discard pixels, that are not inside the original bound
	// leads to cut of squares, but is more accurate and looks smoother,
	// than rendering the full squares, which leeds to popping
	origMaxBoundScreenSpace = getScreenSpacePosOfPoint(modelViewProjectionMatrix, maxBounds, windowWidth, windowHeight);

    // create bounding box geometry
	vec4 pos = modelViewProjectionMatrix * (gl_in[0].gl_Position + vec4(maxBounds_Off[0],maxBounds_Off[1],0.0,0.0));
	gl_Position = pos;
	EmitVertex();

	pos = modelViewProjectionMatrix * (gl_in[0].gl_Position + vec4(minBounds[0],maxBounds_Off[1],0.0,0.0));
	gl_Position = pos;
	EmitVertex();

	pos = modelViewProjectionMatrix * (gl_in[0].gl_Position + vec4(maxBounds_Off[0],minBounds[1],0.0,0.0));
	gl_Position = pos;
	EmitVertex();

	pos = modelViewProjectionMatrix * (gl_in[0].gl_Position + vec4(minBounds[0],minBounds[1],0.0,0.0));
	gl_Position = pos;
	EmitVertex();

	EndPrimitive();
}