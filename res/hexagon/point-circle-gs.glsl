#version 450
#include "/defines.glsl"
#include "/globals.glsl"

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in VS_OUT{
	float pointDiscrepancy;
}gs_in[];

uniform mat4 modelViewProjectionMatrix;

uniform float radius;
uniform float aspectRatio;

//imposter space coordinates
out vec2 isCoords;

out GS_OUT{
	float pointDiscrepancy;
}gs_out;

void main()
{

	gs_out.pointDiscrepancy = gs_in[0].pointDiscrepancy;

    // create bounding box geometry
	vec4 pos = modelViewProjectionMatrix * gl_in[0].gl_Position + vec4(radius, radius/aspectRatio,0.0,0.0);
    isCoords = vec2(1, 1);
	gl_Position = pos;
	EmitVertex();

	pos = modelViewProjectionMatrix * gl_in[0].gl_Position + vec4(-radius,radius/aspectRatio,0.0,0.0);
    isCoords = vec2(-1, 1);
	gl_Position = pos;
	EmitVertex();

	pos = modelViewProjectionMatrix * gl_in[0].gl_Position + vec4(radius, -radius/aspectRatio,0.0,0.0);
    isCoords = vec2(1, -1);
	gl_Position = pos;
	EmitVertex();

	pos = modelViewProjectionMatrix * gl_in[0].gl_Position + vec4(-radius, -radius/aspectRatio,0.0,0.0);
	isCoords = vec2(-1, -1);
    gl_Position = pos;
	EmitVertex();

	EndPrimitive();
}