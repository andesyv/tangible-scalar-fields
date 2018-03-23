#version 420

uniform mat4 modelView;
uniform mat4 projection;
uniform float radiusOffset;

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in int vertexId[];
out vec4 gFragmentPosition;
out vec4 gSpherePosition;
out float gSphereRadius;
flat out uint gSphereId;
flat out int gVertexId;

struct Element
{
	vec3 color;
	float radius;
};

layout(std140, binding = 0) uniform elementBlock
{
	Element elements[32];
};

void main()
{
	uint sphereId = floatBitsToUint(gl_in[0].gl_Position.w);
	uint elementId = bitfieldExtract(sphereId,0,8);
	float sphereRadius = elements[elementId].radius+radiusOffset;
	
	gSphereId = sphereId;
	gSpherePosition = gl_in[0].gl_Position;
	gSphereRadius = sphereRadius;
	gVertexId = vertexId[0];

	vec4 p0 = modelView * vec4(gl_in[0].gl_Position.xyz,1.0);
	vec4 p1 = modelView * (vec4(gl_in[0].gl_Position.xyz,1.0)+vec4(sphereRadius,sphereRadius,sphereRadius,0.0));	
	float radius = length(p1.xyz-p0.xyz);

	vec3 up = vec3(0.0, 1.0, 0.0) * radius;//*sqrt(3.0)*0.5;
	vec3 right = vec3(1.0, 0.0, 0.0) * radius;//*sqrt(3.0)*0.5;

	gFragmentPosition = projection*vec4(p0.xyz - right - up, 1.0);
	gl_Position = gFragmentPosition;
	EmitVertex();

	gFragmentPosition = projection*vec4(p0.xyz + right - up, 1.0);
	gl_Position = gFragmentPosition;
	EmitVertex();

	gFragmentPosition = projection*vec4(p0.xyz - right + up, 1.0);
	gl_Position = gFragmentPosition;
	EmitVertex();

	gFragmentPosition = projection*vec4(p0.xyz + right + up, 1.0);
	gl_Position = gFragmentPosition;
	EmitVertex();

	EndPrimitive();
}