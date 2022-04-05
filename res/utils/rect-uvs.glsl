#iChannel1 "file://resources/rotated_sphere.png"

vec2 rectUvs(ivec2 texSize, vec2 uv) {
    float xmin = 0.5 - float(texSize.y) / float(texSize.x + texSize.x);
    return vec2((1.0 - xmin - xmin) * uv.x + xmin, uv.y);
}

void mainImage(out vec4 fragColor, in vec2 texCoords) {
    vec2 mousePos = iMouse.xy / iResolution.xy;
    vec2 uv = texCoords.xy / iResolution.xy;

    ivec2 texSize = textureSize(iChannel1, 0);
    float squariness = 1.0 - abs(1.0 - pow(iResolution.x/iResolution.y, 4.0));
    fragColor = vec4(texture(iChannel1, rectUvs(texSize, uv)).rgb * squariness, 1.0);
}