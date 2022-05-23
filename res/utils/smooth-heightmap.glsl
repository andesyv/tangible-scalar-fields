#define P 28.0

void mainImage(out vec4 fragColor, in vec2 texCoords) {
    vec3[3] maximas = vec3[3](
        vec3(0.275, 0.725, 1.0),
        vec3(0.6, 0.575, 0.4), 
        vec3(0.575, 0.275, 0.15)
    );

    vec2 uv = texCoords / iResolution.xy;
    
    float dist = 100.0;
    int d_i = 0;
    for (int i = 0; i < 3; i++) {
        float d = length(uv - maximas[i].xy);
        if (d < dist) {
            dist = d;
            d_i = i;
        }
    }


    if (dist < 1.0) {
        fragColor = vec4(vec3(pow(smoothstep(0.0, 1.0, 1.0 - dist), P)) * maximas[d_i].z, 1.0);
        return;
    }

    fragColor = vec4(0.0, 0.0, 0.0, 1.0);
}