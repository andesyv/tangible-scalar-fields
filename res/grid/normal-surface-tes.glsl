#version 450 core

layout (quads, equal_spacing, ccw) in;
//layout(triangles, equal_spacing, ccw) in;

in vec3 tPos[];
in vec2 tTexCoord[];

layout(binding = 1) uniform sampler2D kdeTexture;
layout(std430, binding = 0) buffer genVertexBuffer
{
    vec3 vertices[];
};
uniform mat4 MVP = mat4(1.0);
uniform uint tesselation = 16;

out vec2 uv;

// computes the normal value of a point on a height field
// res/tiles/globals.glsl:118
vec3 calculateNormal(vec2 texCoord, vec2 sampleScale){

    // texelFetch (0,0) -> left bottom
    float top = texture(kdeTexture, texCoord + vec2(0.0, sampleScale.y)).r;
    float bottom = texture(kdeTexture, texCoord + vec2(0.0, -sampleScale.y)).r;
    float left = texture(kdeTexture, texCoord + vec2(-sampleScale.x, 0.0)).r;
    float right = texture(kdeTexture, texCoord + vec2(sampleScale.x, 0.0)).r;

    // reconstruct normal
    vec3 normal = vec3((left-right)/2,(bottom-top)/2, 1);
    return normalize(normal);
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

    const uint v_index = tesselation * int(texCoord.y * tesselation) + int(texCoord.x * tesselation);

    // retrieve control point position coordinates
    vec3 p00 = tPos[0];
    vec3 p01 = tPos[1];
    vec3 p10 = tPos[2];
    vec3 p11 = tPos[3];

    // bilinearly interpolate position coordinate across patch
    vec3 p0 = (p01 - p00) * gl_TessCoord.x + p00;
    vec3 p1 = (p11 - p10) * gl_TessCoord.x + p10;
    vec3 p = (p1 - p0) * gl_TessCoord.y + p0;

    // Displace position with normal:
    vec3 normal = calculateNormal(texCoord, vec2(0.01)) * 0.8;
    p += normal;

    vertices[v_index] = p;
    uv = texCoord;
    gl_Position = MVP * vec4(p, 1.0);

//    gl_Position = gl_TessCoord.x * gl_in[0].gl_Position + gl_TessCoord.y * gl_in[1].gl_Position + gl_TessCoord.z * gl_in[2].gl_Position;
}