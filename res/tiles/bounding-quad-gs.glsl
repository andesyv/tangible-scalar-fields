#version 450
#include "/defines.glsl"
#include "/globals.glsl"

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

//[x,y]
uniform vec2 maxBounds_tiles;
uniform vec2 minBounds_tiles;

uniform vec2 maxBounds_acc;
uniform vec2 minBounds_acc;

uniform int windowHeight;
uniform int windowWidth;

uniform mat4 modelViewProjectionMatrix;

out vec4 boundsScreenSpace;

void main()
{

    boundsScreenSpace = getScreenSpacePosOfRect(modelViewProjectionMatrix, maxBounds_acc, minBounds_acc, windowWidth, windowHeight);

    // create bounding box geometry
	vec4 pos = modelViewProjectionMatrix * (gl_in[0].gl_Position + vec4(maxBounds_tiles[0],maxBounds_tiles[1],0.0,0.0));
	gl_Position = pos;
	EmitVertex();

	pos = modelViewProjectionMatrix * (gl_in[0].gl_Position + vec4(minBounds_tiles[0],maxBounds_tiles[1],0.0,0.0));
	gl_Position = pos;
	EmitVertex();

	pos = modelViewProjectionMatrix * (gl_in[0].gl_Position + vec4(maxBounds_tiles[0],minBounds_tiles[1],0.0,0.0));
	gl_Position = pos;
	EmitVertex();

	pos = modelViewProjectionMatrix * (gl_in[0].gl_Position + vec4(minBounds_tiles[0],minBounds_tiles[1],0.0,0.0));
	gl_Position = pos;
	EmitVertex();

	EndPrimitive();
}