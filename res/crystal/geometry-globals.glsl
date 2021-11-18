#include "/geometry-constants.glsl"

vec3 getRegressionPlaneNormal(ivec2 hexCoord) {
    vec4 tileNormal;
    // get accumulated tile normals from buffer
    for(int i = 0; i < 4; i++){
        tileNormal[i] = float(tileNormals[int((hexCoord.x*(maxTexCoordY+1) + hexCoord.y) * 5 + i)]);
    }
    tileNormal /= bufferAccumulationFactor;// get original value after accumulation

    return normalize(vec3(tileNormal.x, tileNormal.y, tileNormal.w));
}

/// Can use the same function to find offset for neighbor and current hex, because offset is just a 2d vector (with tilt for normal)
vec3 getOffset(uint i, vec3 normal) {
    float angle_rad = HEX_ANGLE * float(gl_LocalInvocationID.x + i);

    // Offset from center of point + grid width
    vec3 offset = vec3(tile_scale * cos(angle_rad), tile_scale * sin(angle_rad), 0.0);
    if (tileNormalsEnabled) {
        float normalDisplacement = dot(-offset, normal) * normal.z;
        if (!isnan(normalDisplacement))
        offset.z += normalDisplacement * tileNormalDisplacementFactor;
    }
    return offset;
}