#version 450
#include "/defines.glsl"
#include "/globals.glsl"

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

//[x,y]
uniform vec2 maxBounds_acc;
uniform vec2 minBounds_acc;

uniform mat4 modelViewProjectionMatrix;

#ifdef RENDER_TILES
	uniform int windowHeight;
	uniform int windowWidth;

	out vec4 boundsScreenSpace;
#endif

//if RENDER_HEXAGONS is defined RENDER_TILES is also defined -> we have windowHeigt, windowWidth
#ifdef RENDER_HEXAGONS
	uniform vec2 rectSize;
	out vec2 rectSizeScreenSpace;
#endif

void main()
{

	#ifdef RENDER_TILES
    	boundsScreenSpace = getScreenSpacePosOfRect(modelViewProjectionMatrix, maxBounds_acc, minBounds_acc, windowWidth, windowHeight);
	#endif

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