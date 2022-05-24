#define P 16.0

void mainImage(out vec4 fragColor, in vec2 texCoords) {
    vec2 mousePos = iMouse.xy / iResolution.xy;
    vec3[4] maximas = vec3[4](
        // vec3(0.377, 0.518, 0.6),
        vec3(mousePos, 0.6),
        vec3(0.629, 0.226, 1.0), 
        vec3(0.498, 0.262, 0.4),
        vec3(0.665, 0.426, 0.14)
    );

    vec2 uv = texCoords / iResolution.xy;
    
    bool value = false;
    float dist = 0.0;
    float d_a = 0.0;
    for (int i = 0; i < 4; i++) {
        float d = length(uv - maximas[i].xy);
        if (d < 1.0) {
            d = pow(smoothstep(0.0, 1.0, 1.0 - d), P);
            if (!value) {
                dist = d;
                d_a = maximas[i].z;
                value = true;
            } else {
                dist = 0.5 * dist + 0.5 * d;
                d_a = 0.5 * d_a + 0.5 * maximas[i].z;
            }
        }
    }


    if (value) {
        fragColor = vec4(vec3(dist), 1.0);
        return;
    }

    fragColor = vec4(0.0, 0.0, 0.0, 1.0);
}