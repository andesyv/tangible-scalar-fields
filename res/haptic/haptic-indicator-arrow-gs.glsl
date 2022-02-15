#version 450

/**
 * Arrow geometry shader copied from own previous work
 */

layout(points) in;
layout(line_strip, max_vertices = 5) out;

uniform mat4 P = mat4(1.0);
uniform mat4 PInv = mat4(1.0);
uniform vec3 view_dir; // In view coordinates
uniform float scale = 1.0;

vec3 rotate(vec3 inp, float angle) {
    mat3 T = mat3(
        vec3(cos(angle), -sin(angle), 0),
        vec3(sin(angle), cos(angle), 0),
        vec3(0, 0, 1)
    );
    return T * inp;
}

void main(void)
{
    // Find length from near clip-plane to geometry to use as uniform scale bias
    vec4 v_near = PInv * vec4(0.0, 0., -1.0, 1.);
    v_near /= v_near.w;
    vec4 v_far = PInv * gl_in[0].gl_Position;
    v_far /= v_far.w;
    const float view_scale = length((v_far - v_near).xyz);

    gl_Position = gl_in[0].gl_Position;
    EmitVertex();

    vec4 translation = P * vec4(normalize(view_dir) * scale, 0.0);
    vec4 p1 = gl_in[0].gl_Position + translation;
    gl_Position = p1;
    EmitVertex();

    translation = P * vec4(normalize(rotate(view_dir, 0.1)) * 0.9 * scale, 0.0);
    gl_Position = gl_in[0].gl_Position + translation;
    EmitVertex();

    translation = P * vec4(normalize(rotate(view_dir, -0.1)) * 0.9 * scale, 0.0);
    gl_Position = gl_in[0].gl_Position + translation;
    EmitVertex();

    gl_Position = p1;
    EmitVertex();


    EndPrimitive();
}