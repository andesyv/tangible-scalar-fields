#version 450 core

in flat vec4 centerFragment;
in vec4 fragPos;

uniform float radius = 1.0;
uniform mat4 PInv = mat4(1.0);

out vec4 fragColor;

void main() {
    vec4 worldFragDir = PInv * vec4(fragPos.xy - centerFragment.xy, 0., 0.);
    if (radius < length(worldFragDir.xyz))
        discard;

    fragColor = vec4(0.0, 1.0, 0.0, 1.0);
}