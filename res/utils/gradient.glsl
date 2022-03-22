void mainImage(out vec4 fragColor, in vec2 texCoords) {
    vec2 uv = texCoords / iResolution.xy;

    fragColor = vec4(vec3(uv.x * uv.y), 1.0);
}