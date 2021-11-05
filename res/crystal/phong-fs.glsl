#version 450

in vec3 normal;
in vec3 gFragPos;

uniform vec3 lightPos = vec3(0.0);

out vec4 fragColor;

void main() {
    vec3 lightDir = normalize(gFragPos - lightPos);
//    vec3 viewDir = normalize(viewPos - gFragPos);
    vec3 n = normalize(normal);

//    vec3 H = normalize(-lightDir + viewDir);
//    float specular = pow(max(dot(n, H), 0.0), 128.0);
    vec3 phong = max(0.15, dot(n, lightDir)) * vec3(0.0, 0.7, 1.0);

    fragColor = vec4(phong, 1.0);
}