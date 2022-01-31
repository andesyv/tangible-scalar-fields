#version 450

layout(pixel_center_integer) in vec4 gl_FragCoord;
in vec2 uv;

layout (binding = 0) uniform sampler2D normalTex;

out vec4 fragColor;

//vec3 getRegressionPlaneNormal(ivec2 hexCoord) {
//    vec3 tileNormal;
//    // get accumulated tile normals from buffer
//    for(int i = 0; i < 3; i++){
//        tileNormal[i] = float(tileNormals[int((hexCoord.x*(maxTexCoordY+1) + hexCoord.y) * 5 + i)]);
//    }
//    tileNormal /= bufferAccumulationFactor;// get original value after accumulation
//
//    return normalize(tileNormal);
//}

void main() {
    fragColor = vec4(texture(normalTex, uv).rgb, 1.0);
}