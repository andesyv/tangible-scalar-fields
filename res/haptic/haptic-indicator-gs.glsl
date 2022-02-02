#version 450 core

/**
 * Makes a rough bounding box around a sceen-spaced circle
 */

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

uniform mat4 P = mat4(1.0);
uniform float radius = 1.0;

out flat vec4 centerFragment;
out vec4 fragPos;

void main() {
    centerFragment = gl_in[0].gl_Position;

    fragPos = gl_in[0].gl_Position + P * (vec4(-1.0, 1.0, 0., 0.) * radius);
    gl_Position = fragPos;
    EmitVertex();

    fragPos = gl_in[0].gl_Position + P * (vec4(-1.0, -1.0, 0., 0.) * radius);
    gl_Position = fragPos;
    EmitVertex();

    fragPos = gl_in[0].gl_Position + P * (vec4(1.0, 1.0, 0., 0.) * radius);
    gl_Position = fragPos;
    EmitVertex();

    fragPos = gl_in[0].gl_Position + P * (vec4(1.0, -1.0, 0., 0.) * radius);
    gl_Position = fragPos;
    EmitVertex();

    EndPrimitive();
}