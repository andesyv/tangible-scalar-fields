#version 450

flat in uint gs_id;

uniform int POINT_COUNT = 1;

out vec4 fragColor;

void main() {
    vec3 col = vec3(float(gs_id) / POINT_COUNT, mod(float(gs_id), POINT_COUNT / 2), mod(float(gs_id), POINT_COUNT / 3));
    fragColor = vec4(col, 1.0);
}