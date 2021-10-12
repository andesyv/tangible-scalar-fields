#version 450

in vec2 inPos;

uniform mat4 MVP = mat4(1.0);

void main() {
    gl_Position = MVP * vec4(inPos.xy, 0., 1.0);
}