#define P 20.0

void mainImage(out vec4 fragColor, in vec2 texCoords) {
    vec2[3] maximas = vec2[3](
        vec2(0.546, 0.219),
        vec2(0.604, 0.629), 
        vec2(0.253, 0.454)
    );

    vec2[2] minimas = vec2[2](
        vec2(0.335, 0.277),
        vec2(0.625, 0.396)
    );

    vec2 uv = texCoords / iResolution.xy;
    
    float d_max = 100.0;
    float d_min = 100.0;
    for (int i = 0; i < 3; i++) {
        float d = length(uv - maximas[i]);
        if (d < d_max)
            d_max = d;
    }
    for (int i = 0; i < 2; i++) {
        float d = length(uv - minimas[i]);
        if (d < d_min)
            d_min = d;
    }

    if (1.0 < d_max && 1.0 < d_min) {
        fragColor = vec4(vec3(.5), 1.0);
        return;
    }

    d_min = smoothstep(0.0, 1.0, pow(1.0 - clamp(d_min, 0.0, 1.0), P))+1.0;
    d_max = smoothstep(0.0, 1.0, pow(1.0 - clamp(d_max, 0.0, 1.0), P))+1.0;
    // [0.5, 1, 2] -> [0, 1]: -0.5 / 1.5
    float t = (d_min / d_max - 0.5) / 1.5;
    fragColor = vec4(vec3(mix(0.0, 1.0, t)), 1.0);
}