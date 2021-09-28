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
    // hexagon-tiles-fs.glsl: 109
    const float max = uintBitsToFloat(maxAccumulate) + 1;

    vec3 col = vec3(fs_in.value / max, fract(2.3 * fs_in.value / max), fract(2.4 * fs_in.value / max));
    fragColor = vec4(col, 1.0);
}