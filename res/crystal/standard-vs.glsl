#version 450

in vec4 inPos;

uniform mat4 MVP = mat4(1.0);

out flat uint id_vs;

void main() {
    id_vs = uint(float(gl_VertexID) / 3.0);
    if (inPos.w < 1.0)
        return;
    gl_Position = MVP * vec4(inPos.xyz, 1.0);
}