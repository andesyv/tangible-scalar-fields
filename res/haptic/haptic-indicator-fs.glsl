#version 450 core

in flat vec4 centerFragment;
in vec4 fragPos;

uniform float radius = 1.0;
uniform mat4 PInv = mat4(1.0);

out vec4 fragColor;

void main() {
    vec4 fragDir = PInv * vec4(fragPos.xyz - centerFragment.xyz, 0.);
    float fragDirLen = length(fragDir.xyz);
    if (radius < fragDirLen)
        discard;

    // Simulate a ball normal using some trigonometry
    vec3 normal = normalize(fragDir.xyz + vec3(0., 0., radius * sin(acos(fragDirLen / radius))));

    const vec3 lightDir = vec3(0.0, 0.0, 1.0);
    vec3 color = max(dot(normal, lightDir), 0.15) * vec3(0.1, 1.0, 0.3);

    fragColor = vec4(color, 1.0);
}