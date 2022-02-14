#version 450 core

in vec2 uv;

uniform mat4 MVP = mat4(1.0);
uniform float mip_map_level = 0.0;
layout(binding = 0) uniform sampler2D normalTex;

out vec4 fragColor;

vec2 aspectRatioCorrectedTexCoords(ivec2 texSize, vec2 texCoords) {
    float ratio = float(texSize.x) / float(texSize.y);
    return vec2((texCoords.x * 2.0 - 1.0) / ratio * 0.5 + 0.5, texCoords.y);
}

void main() {
    const ivec2 texSize = textureSize(normalTex, 0);
    vec3 normal = textureLod(normalTex, uv, mip_map_level).rgb * 2.0 - 1.0;
//    vec3 p_normal = (MVP * vec4(normal, 0.0)).xyz;
//    float phong = max(dot(p_normal, vec3(1., 0., 0.)), 0.15);
    fragColor = vec4(normal, 1.0);
}