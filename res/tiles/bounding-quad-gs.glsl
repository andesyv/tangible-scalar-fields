#version 450
#include "/defines.glsl"
#include "/globals.glsl"

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

//[x,y]
uniform vec2 maxBounds_acc;
uniform vec2 minBounds_acc;

uniform int windowHeight;
uniform int windowWidth;

uniform mat4 modelViewProjectionMatrix;

out vec4 boundsScreenSpace;

#ifdef RENDER_HEXAGONS
	uniform vec2 rectSize;
	out vec2 rectSizeScreenSpace;
#endif

void main()
{

    boundsScreenSpace = getScreenSpacePosOfRect(modelViewProjectionMatrix, maxBounds_acc, minBounds_acc, windowWidth, windowHeight);

	#ifdef RENDER_HEXAGONS
		rectSizeScreenSpace = getScreenSpacePosOfPoint(modelViewProjectionMatrix, rectSize * vec2(2,2), windowWidth, windowHeight)-getScreenSpacePosOfPoint(modelViewProjectionMatrix, rectSize, windowWidth, windowHeight);
	#endif

    // create bounding box geometry
	vec4 pos = modelViewProjectionMatrix * (gl_in[0].gl_Position + vec4(maxBounds_acc[0],maxBounds_acc[1],0.0,0.0));
	gl_Position = pos;
	EmitVertex();

	pos = modelViewProjectionMatrix * (gl_in[0].gl_Position + vec4(minBounds_acc[0],maxBounds_acc[1],0.0,0.0));
	gl_Position = pos;
	EmitVertex();

	pos = modelViewProjectionMatrix * (gl_in[0].gl_Position + vec4(maxBounds_acc[0],minBounds_acc[1],0.0,0.0));
	gl_Position = pos;
	EmitVertex();

	pos = modelViewProjectionMatrix * (gl_in[0].gl_Position + vec4(minBounds_acc[0],minBounds_acc[1],0.0,0.0));
	gl_Position = pos;
	EmitVertex();

	EndPrimitive();
}