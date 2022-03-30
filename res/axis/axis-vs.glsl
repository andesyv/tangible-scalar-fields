#version 450 core

in vec3 inPos;
in vec3 inColor;

uniform mat4 MVP = mat4(1.0);

out vec3 vColor;

void main() {
    vColor = inColor;
    gl_Position = MVP * vec4(inPos, 1.0);
}