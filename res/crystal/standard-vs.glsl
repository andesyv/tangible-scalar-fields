#version 450

in vec4 inPos;
out vec3 fragPos;

uniform mat4 MVP = mat4(1.0);

void main() {
    if (inPos.w < 1.0)
        return;

    fragPos = inPos.xyz;

    gl_Position = MVP * vec4(inPos.xyz, 1.0);
}