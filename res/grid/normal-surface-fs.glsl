#version 450 core

in vec2 uv;

layout(binding = 0) uniform sampler2D normalTex;

out vec4 fragColor;

void main() {
    vec3 normal = texture(normalTex, uv).rgb * 2.0 - 1.0;
    fragColor = vec4(normal, 1.0);
}