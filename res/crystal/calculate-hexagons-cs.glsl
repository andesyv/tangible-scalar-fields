#version 450

layout(local_size_x = 6) in;

layout(std430, binding = 0) buffer genVertexBuffer
{
    vec4 vertices[];
};

uniform int num_cols = 4;
uniform int num_rows = 4;
uniform float height = 1.0;
uniform float tile_scale = 0.6;
uniform mat4 disp_mat = mat4(1.0);
uniform uint POINT_COUNT = 1u;
layout(binding = 1) uniform sampler2D accumulateTexture;

layout(std430, binding = 2) buffer valueMaxBuffer
{
    uint maxAccumulate;
    uint maxPointAlpha;
};

const float PI = 3.1415926;

void main() {
    // Common for whole work group:
    /// Could make one local invocation do all of this common work for the whole group
    const uint hexID =
        gl_WorkGroupID.x +
        gl_WorkGroupID.y * gl_NumWorkGroups.x +
        gl_WorkGroupID.z * gl_NumWorkGroups.x * gl_NumWorkGroups.y;
    // Early quit if this invocation is outside range
    if (POINT_COUNT <= hexID)
        return;

    //calculate position of hexagon center - in double height coordinates!
    //https://www.redblobgames.com/grids/hexagons/#coordinates-doubled
    const float col = mod(hexID,num_cols);
    const float row = floor(hexID/num_cols) * 2.0 - (mod(col,2) == 0 ? 0.0 : 1.0);

    // get value from accumulate texture
    ivec2 tilePosInAccTexture = ivec2(col, floor(hexID/num_cols));
    float hexValue = texelFetch(accumulateTexture, tilePosInAccTexture, 0).r;

    // hexagon-tiles-fs.glsl: 109
    const float max = uintBitsToFloat(maxAccumulate) + 1;

    float depth = 2.0 * height * hexValue / max - height; // [0,maxAccumulate] -> [-1, 1]

    vec4 centerPos = disp_mat * vec4(col, row, depth, 1.0);
    centerPos /= centerPos.w;


    // Individual for each work group:
    // Triangles are 3 vertices, and a hexagon is 6 triangles, so skip by 6*3 per hexagon.
    // Triangles are 3 vertices, so skip by 3 for each local invocation
    const uint triangleIndex = hexID * 3 * 6 + gl_LocalInvocationID.x * 3;

    vertices[triangleIndex] = centerPos;
    for (uint i = 0; i < 2u; ++i) {
        float angle_deg = 60.0 * float(gl_LocalInvocationID.x + i);
        float angle_rad = PI / 180.0 * angle_deg;

        // Offset from center of point + grid width
        vec3 offset = vec3(tile_scale * cos(angle_rad), tile_scale * sin(angle_rad), 0.0);

        vertices[triangleIndex + i + 1] = vec4(centerPos.xyz + offset, 1.0);
    }
}