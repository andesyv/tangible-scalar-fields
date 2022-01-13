#version 450

in vec3 inPos;

uniform int num_cols = 4;
uniform int num_rows = 4;
uniform float height = 1.0;
uniform mat4 disp_mat = mat4(1.0);
layout(binding = 0) uniform sampler2D accumulateTexture;
uniform float tile_scale = 0.6;

const float PI = 3.1415926;

layout(std430, binding = 1) buffer valueMaxBuffer
{
    uint maxAccumulate;
    uint maxPointAlpha;
};

out VS_OUT {
    uint id;
    ivec2 texPos;
    float value;
    vec3 vertices[7];
} vs_out;

void main() {
    //calculate position of hexagon center - in double height coordinates!
    //https://www.redblobgames.com/grids/hexagons/#coordinates-doubled
    float col = mod(gl_InstanceID,num_cols);
    float row = floor(gl_InstanceID/num_cols) * 2.0 - (mod(col,2) == 0 ? 0.0 : 1.0);

    vs_out.id = gl_InstanceID;

    // position of hexagon in accumulate texture (offset coordinates)
    ivec2 tilePosInAccTexture = ivec2(col, floor(gl_InstanceID/num_cols));
    vs_out.texPos = tilePosInAccTexture;

    // get value from accumulate texture
    float hexValue = texelFetch(accumulateTexture, tilePosInAccTexture, 0).r;
    vs_out.value = hexValue;

    // hexagon-tiles-fs.glsl: 109
    const float max = uintBitsToFloat(maxAccumulate) + 1;

    float depth = 2.0 * height * hexValue / max - height; // [0,maxAccumulate] -> [-1, 1]

    // See CrystalRenderer.cpp: 156
    // gl_Position =  vec4(col * horizontal_space + minBounds_Off.x + tileSize, row * vertical_space/2.0f + minBounds_Off.y + vertical_space, 0.0f, 1.0f);
    // gl_Position = vec4((col - 0.5 * (num_cols-1)) * horizontal_space, vertical_space * 0.5 * (row - num_rows + (num_cols != 1 ? 1.5 : 1.0)), 0.0, 1.0);
    vec4 centerPos = disp_mat * vec4(col, row, depth, 1.0);

    // Generate out vertices:
    uint vertexIndex = gl_InstanceID * 7;
    vs_out.vertices[0] = centerPos.xyz;
    for (int i = 0; i < 6; ++i) {
        //flat-topped
        float angle_deg = -60.0 * i;
        float angle_rad = PI / 180.0 * angle_deg;

        // Offset from center of point + grid width
        vec4 offset = vec4(tile_scale * cos(angle_rad), tile_scale * sin(angle_rad), 0.0, 0.0);
        vs_out.vertices[i+1] = (centerPos + offset).xyz;
    }

    gl_Position = centerPos;
}