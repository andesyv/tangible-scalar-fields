#version 450

in flat uint id_vs;

out vec4 fragColor;

void main() {
    const float max = 10.0;
    uint id_triangle = id_vs / 6;
    if (9000 < id_vs)
        discard;
    vec3 col = vec3(float(id_triangle) / max, fract(2.3 * float(id_triangle) / max), fract(2.4 * float(id_triangle) / max));
    fragColor = vec4(col, 1.0);
}