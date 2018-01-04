#version 400

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;
in vec4 tePatchDistance[3];
in vec3 tePosition[3];
out vec4 gPatchDistance;
out vec3 gPosition;

uniform mat4 modelView;

void main()
{   
    gPatchDistance = tePatchDistance[0];
	gPosition = tePosition[0];
    gl_Position = gl_in[0].gl_Position;
	EmitVertex();

    gPatchDistance = tePatchDistance[1];
	gPosition = tePosition[1];
    gl_Position = gl_in[1].gl_Position;
	EmitVertex();

    gPatchDistance = tePatchDistance[2];
	gPosition = tePosition[2];
    gl_Position = gl_in[2].gl_Position;
	EmitVertex();

    EndPrimitive();
}