#version 450 core

in vec2 uv;

uniform mat4 MVP = mat4(1.0);
uniform float mip_map_level = 0.0;
uniform float opacity = 1.0;
layout(binding = 0) uniform sampler2D normalTex;

out vec4 fragColor;

vec2 aspectRatioCorrectedTexCoords(ivec2 texSize, vec2 texCoords) {
    float ratio = float(texSize.x) / float(texSize.y);
    return vec2((texCoords.x * 2.0 - 1.0) / ratio * 0.5 + 0.5, texCoords.y);
}

// HSL/HSV conversion functions taken from http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl by Sam Hocevar and Emil Persson
vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = c.g < c.b ? vec4(c.bg, K.wz) : vec4(c.gb, K.xy);
    vec4 q = c.r < p.x ? vec4(p.xyw, c.r) : vec4(c.r, p.yzx);

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

bool isZero(vec3 normal) {
    const float EPS = 0.01;
    return -EPS < normal.x && normal.x < EPS && -EPS < normal.y && normal.y < EPS && -EPS < normal.z && normal.z < EPS;
}

void main() {
    const ivec2 texSize = textureSize(normalTex, 0);
//    float height = textureLod(normalTex, uv, mip_map_level).a;
//    fragColor = vec4(vec3(height), 1.0);
//    return;
    vec3 normal = textureLod(normalTex, uv, mip_map_level).rgb;
    if (isZero(normal * 2.0 - 1.0))
        normal = vec3(0.5, 0.5, 1.0);
    //    vec3 p_normal = (MVP * vec4(normal, 0.0)).xyz;
    //    float phong = max(dot(p_normal, vec3(1., 0., 0.)), 0.15);
    vec3 hsv = rgb2hsv(normal);
    hsv.r *= opacity;
    fragColor = vec4(hsv2rgb(hsv), 0.4);
    //    fragColor = vec4(floor(uv * 10.0) * 0.1, 0.0, 1.0);
}