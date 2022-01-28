#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inTexCoord;

uniform mat4 MVP = mat4(1.0);

out vec3 vPos;
out vec2 vTexCoord;

void main() {
    vTexCoord = inTexCoord;
    vPos = inPos;
    gl_Position = MVP * vec4(inPos, 1.0);
}