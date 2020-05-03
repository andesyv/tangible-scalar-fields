#version 450
#include "/defines.glsl"
#include "/globals.glsl"

uniform sampler2D kdeTexture;

layout(location = 0) out vec4 densityNormalsTexture;

void main(){
    densityNormalsTexture = vec4(calculateNormal(ivec2(gl_FragCoord.xy),kdeTexture), 1.0f);
}


