#version 450

out vec4 fragColor;

float linearizeDepth(float ndc, float near, float far) {
    return (2.0 * near * far) / (far + near - ndc * (far - near));
}

void main() {
    fragColor = vec4(vec3(max(1.0 - linearizeDepth(gl_FragCoord.z, 0.1, 1.0), 0.0)), 1.0);
}