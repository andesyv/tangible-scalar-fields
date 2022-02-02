#version 450 core

in vec3 inPos;

uniform mat4 MVP = mat4(1.0);

void main() {
    gl_Position = MVP * vec4(inPos, 1.0);
}