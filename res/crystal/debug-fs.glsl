#version 450

layout(pixel_center_integer) in vec4 gl_FragCoord;
in vec2 uv;

const float bufferAccumulationFactor = 100.0;

uniform int num_cols = 0;
uniform int num_rows = 0;
uniform int maxTexCoordY;

layout(std430, binding = 0) buffer tileNormalsBuffer
{
    int tileNormals[];
};

out vec4 fragColor;

vec3 getRegressionPlaneNormal(ivec2 hexCoord) {
    vec3 tileNormal;
    // get accumulated tile normals from buffer
    for(int i = 0; i < 3; i++){
        tileNormal[i] = float(tileNormals[int((hexCoord.x*(maxTexCoordY+1) + hexCoord.y) * 5 + i)]);
    }
    tileNormal /= bufferAccumulationFactor;// get original value after accumulation

    return normalize(tileNormal);
}

// https://www.redblobgames.com/grids/hexagons/#conversions-doubled
ivec2 axial_to_hex(ivec2 ax) {
    return ivec2(ax.x * 2 + ax.y, ax.y);
}

/**
 * https://www.redblobgames.com/grids/hexagons/#rounding
 * axial_round(p) = cube_to_axial(cube_round(axial_to_cube(p)))
 */
ivec2 axial_round(vec2 p) {
    vec3 cb = vec3(p.x, p.y, -p.x-p.y);
    vec3 i = round(cb);
    vec3 f = abs(i - cb);

    if (f.x > f.y && f.x > f.z) {
        return ivec2(-i.y-i.z, i.y);
    } else if (f.y > f.z) {
        return ivec2(i.x, -i.x-i.z);
    } else {
        return ivec2(i.x, i.y);
    }
}

const float SQRT3_3 = sqrt(3.0) / 3.0;

// https://www.redblobgames.com/grids/hexagons/#pixel-to-hex
vec2 pixel_to_axial(vec2 p) {
    const float size = 1.0 / num_cols;
    float q = (SQRT3_3 * p.x - p.y / 3.0) / size;
    float r = (2.0 * p.y / 3.0) / size;

    return vec2(q, r);
}

void main() {
    // get hexagon coordinates of fragment
    ivec2 hex = axial_to_hex(axial_round(pixel_to_axial(uv)));
    vec3 normal = getRegressionPlaneNormal(hex);

    fragColor = vec4(hex / vec2(num_cols, num_rows), 0., 1.0);
}