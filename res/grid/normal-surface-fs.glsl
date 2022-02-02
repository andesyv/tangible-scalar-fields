#version 450 core

in vec2 uv;

layout(binding = 0) uniform sampler2D normalTex;

out vec4 fragColor;

vec2 aspectRatioCorrectedTexCoords(ivec2 texSize, vec2 texCoords) {
    float ratio = float(texSize.x) / float(texSize.y);
    return vec2((texCoords.x * 2.0 - 1.0) / ratio * 0.5 + 0.5, texCoords.y);
}

void main() {
    const ivec2 texSize = textureSize(normalTex, 0);
    vec3 normal = texture(normalTex, aspectRatioCorrectedTexCoords(texSize, uv)).rgb * 2.0 - 1.0;
    fragColor = vec4(normal, 1.0);
}