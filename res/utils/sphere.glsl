float random(vec2 st) {
    return fract(sin(dot(st.xy,
                         vec2(12.9898,78.233)))*
        43758.5453123);
}

void mainImage(out vec4 fragColor, in vec2 texCoords) {
    const float sphereScale = 4.0; // default: 2.0
    const float radius = 0.8;

    vec2 mousePos = iMouse.xy / iResolution.xy;
    vec2 uv =   mod(sphereScale * texCoords / iResolution.xx, 2.0) -
                vec2(1.0, iResolution.y / iResolution.x);
    float UVLen = length(uv);
    if (radius < UVLen) {
        fragColor = vec4(vec3(0.0), 1.0);
        return;
    }

    vec3 normal = normalize(vec3(uv, radius * sin(acos(UVLen / radius))));
    float value = dot(vec3(0.0, 0.0, 1.0), normal);
    float r = random(uv);
    float threshold = 2.0 * mousePos.x - 0.5;

    fragColor = vec4(vec3(threshold < r ? value : 0.0), 1.0);
}