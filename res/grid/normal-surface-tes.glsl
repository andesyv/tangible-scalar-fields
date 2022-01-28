#version 450 core

layout (quads, equal_spacing, ccw) in;
//layout(triangles, equal_spacing, ccw) in;

in vec2 tTexCoord[];

uniform mat4 MVP = mat4(1.0);

out vec2 uv;

void main() {
    vec2 t00 = tTexCoord[0];
    vec2 t01 = tTexCoord[1];
    vec2 t10 = tTexCoord[2];
    vec2 t11 = tTexCoord[3];

    // bilinearly interpolate texture coordinate across patch
    vec2 t0 = (t01 - t00) * gl_TessCoord.x + t00;
    vec2 t1 = (t11 - t10) * gl_TessCoord.x + t10;
    vec2 texCoord = (t1 - t0) * gl_TessCoord.y + t0;

    // retrieve control point position coordinates
    vec4 p00 = gl_in[0].gl_Position;
    vec4 p01 = gl_in[1].gl_Position;
    vec4 p10 = gl_in[2].gl_Position;
    vec4 p11 = gl_in[3].gl_Position;

    // bilinearly interpolate position coordinate across patch
    vec4 p0 = (p01 - p00) * gl_TessCoord.x + p00;
    vec4 p1 = (p11 - p10) * gl_TessCoord.x + p10;
    vec4 p = (p1 - p0) * gl_TessCoord.y + p0;

    uv = texCoord;
    gl_Position = p;

//    gl_Position = gl_TessCoord.x * gl_in[0].gl_Position + gl_TessCoord.y * gl_in[1].gl_Position + gl_TessCoord.z * gl_in[2].gl_Position;
}