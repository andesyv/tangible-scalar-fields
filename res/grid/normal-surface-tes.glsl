#version 450 core

layout (quads, equal_spacing, ccw) in;
//layout(triangles, equal_spacing, ccw) in;

in vec3 tPos[];
in vec2 tTexCoord[];

uniform mat4 MVP = mat4(1.0);
uniform uint tesselation = 16;
uniform float mip_map_level = 0.0;
uniform float surface_height = 1.0;
layout(binding = 0) uniform sampler2D normalTex;

out vec2 uv;

vec2 aspectRatioCorrectedTexCoords(ivec2 texSize, vec2 texCoords) {
    float ratio = float(texSize.x) / float(texSize.y);
    return vec2((texCoords.x * 2.0 - 1.0) / ratio * 0.5 + 0.5, texCoords.y);
}

void main() {
    const vec2 t00 = tTexCoord[0];
    const vec2 t01 = tTexCoord[1];
    const vec2 t10 = tTexCoord[2];
    const vec2 t11 = tTexCoord[3];

    // bilinearly interpolate texture coordinate across patch
    const vec2 t0 = (t01 - t00) * gl_TessCoord.x + t00;
    const vec2 t1 = (t11 - t10) * gl_TessCoord.x + t10;
    const vec2 texCoord = (t1 - t0) * gl_TessCoord.y + t0;

    // retrieve control point position coordinates
    vec3 p00 = tPos[0];
    vec3 p01 = tPos[1];
    vec3 p10 = tPos[2];
    vec3 p11 = tPos[3];

    // bilinearly interpolate position coordinate across patch
    vec3 p0 = (p01 - p00) * gl_TessCoord.x + p00;
    vec3 p1 = (p11 - p10) * gl_TessCoord.x + p10;
    vec3 p = (p1 - p0) * gl_TessCoord.y + p0;

    // Tesselation displacement mapping using normal
    const ivec2 texSize = textureSize(normalTex, 0);
    vec4 normal = textureLod(normalTex, texCoord, mip_map_level);
    float normalFactor = normal.a * 0.01;

    p.z += normalFactor * surface_height;

    uv = texCoord;
    gl_Position = MVP * vec4(p, 1.0);

//    gl_Position = gl_TessCoord.x * gl_in[0].gl_Position + gl_TessCoord.y * gl_in[1].gl_Position + gl_TessCoord.z * gl_in[2].gl_Position;
}