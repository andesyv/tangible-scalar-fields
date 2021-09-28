#version 450

in GS_OUT {
    flat uint id;
    flat float value;
} fs_in;

layout(std430, binding = 1) buffer valueMaxBuffer
{
    uint maxAccumulate;
    uint maxPointAlpha;
};

uniform int POINT_COUNT = 1;

out vec4 fragColor;

void main() {
//    vec3 col = vec3(float(gs_id) / POINT_COUNT, mod(float(gs_id), POINT_COUNT / 2), mod(float(gs_id), POINT_COUNT / 3));
    float val = fs_in.value / 10.0;
    fragColor = vec4(vec3(val), 1.0);
}