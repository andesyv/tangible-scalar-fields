#version 450

in vec4 inPos;

out vec2 uv;

void main() {
    uv = inPos.xy * 0.5 + 0.5;

    gl_Position = vec4(inPos.xy, 0., 1.0);
}