#version 450
#include "/defines.glsl"

uniform sampler2D kdeTexture;

layout(location = 0) out vec4 densityNormalsTexture;

// https://stackoverflow.com/questions/49640250/calculate-normals-from-heightmap
vec3 calculateNormal(ivec2 texelCoord){
                
    // texelFetch (0,0) -> left bottom
    float top = texelFetch(kdeTexture, ivec2(texelCoord.x, texelCoord.y+1), 0).r;
    float bottom = texelFetch(kdeTexture, ivec2(texelCoord.x, texelCoord.y-1), 0).r;
    float left = texelFetch(kdeTexture, ivec2(texelCoord.x-1, texelCoord.y), 0).r;
    float right = texelFetch(kdeTexture, ivec2(texelCoord.x+1, texelCoord.y), 0).r;

    // reconstruct normal
    vec3 normal = vec3((right-left)/2,(top-bottom)/2,1);
    return normalize(normal);
}

void main(){
    densityNormalsTexture = vec4(calculateNormal(ivec2(gl_FragCoord.xy)), 1.0f);
}


