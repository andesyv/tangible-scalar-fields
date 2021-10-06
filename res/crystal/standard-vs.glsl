#version 450

in vec4 inPos;

uniform mat4 MVP = mat4(1.0);

void main() {
    if (inPos.w < 1.0)
        return;

    gl_Position = MVP * vec4(inPos.xyz, 1.0);
}