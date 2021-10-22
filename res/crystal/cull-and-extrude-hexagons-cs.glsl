#version 450

layout(local_size_x = 6, local_size_y = 1) in;

layout(std430, binding = 0) buffer vertexBuffer
{
    vec4 vertices[];
};

uniform int num_cols = 4;
uniform int num_rows = 4;
uniform float height = 1.0;
uniform float tile_scale = 0.6;
uniform mat4 disp_mat = mat4(1.0);
uniform uint POINT_COUNT = 1u;
uniform uint HULL_SIZE = 0u;
uniform float extrude_factor = 0.5;
//layout(binding = 1) uniform sampler2D accumulateTexture;

//layout(std430, binding = 2) buffer valueMaxBuffer
//{
//    uint maxAccumulate;
//    uint maxPointAlpha;
//};

layout(std430, binding = 1) buffer hullBuffer
{
    uint convex_hull[]; // Should indicate which hexID's are part of the hull.
//    vec4 convex_hull[];
};

uniform atomic_uint edgeOffset;

const float PI = 3.1415926;
const float HEX_ANGLE = PI / 180.0 * 60.0;
const float EPSILON = 0.0001;

const ivec2 NEIGHBORS[6] = ivec2[6](
    ivec2(1, 1), ivec2(0, 2), ivec2(-1, 1),
    ivec2(-1,-1), ivec2(0, -2), ivec2(1, -1)
);

vec4 getHexCenterPos(uint i) {
    // triangle size (3) * triangles (6) * stride (2 hexagons) = 3 * 6 * 2
    return vertices[3 * 6 * 2 * i];
}

float scalarCross2D(vec2 a, vec2 b) {
    float c = cross(vec3(a, 0.0), vec3(b, 0.0));
    return c.z;
}

bool ccw(vec2 a, vec2 b) {
    return EPSILON <= scalarCross2D(a,b);
}

bool isInsideHull(vec4 pos) {
    for (uint i = 0; i < HULL_SIZE; ++i) {
        uint j = (i + 1) % HULL_SIZE;
        vec2 a = (getHexCenterPos(i) - pos).xy;
        vec2 b = (getHexCenterPos(j) - getHexCenterPos(i)).xy;

        if (!ccw(a,b))
            return false;
    }
    return true;
}

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
    const int col = int(hexID % num_cols);
    const int row = int(hexID/num_cols) * 2 - (col % 2 == 0 ? 0 : 1);

    // get value from accumulate texture
    ivec2 tilePosInAccTexture = ivec2(col, floor(hexID/num_cols));
    float hexValue = texelFetch(accumulateTexture, tilePosInAccTexture, 0).r;

    // hexagon-tiles-fs.glsl: 109
    const float max = uintBitsToFloat(maxAccumulate) + 1;

    float depth = 2.0 * height * hexValue / max - height; // [0,maxAccumulate] -> [-1, 1]

    // vec4 centerPos = disp_mat * vec4(col, row, depth, 1.0);
    //    centerPos /= centerPos.w;

    const uint centerIndex = hexID * 3 * gl_WorkGroupSize.x * 2;
    vec4 centerPos = vertices[centerIndex];
    bool insideHull = isInsideHull(centerPos);



    // Individual for each work group:

    // Triangles are 3 vertices, and a hexagon is 6 triangles + 6 sides, so skip by 2*6*3 per hexagon.
    // Triangles are 3 vertices, so skip by 3 for each local invocation
    const uint triangleIndex = gl_LocalInvocationID.x * 3 + hexID * 3 * gl_WorkGroupSize.x * 2; // gl_WorkGroupSize.y == 2 in previous generation invocation
    const uint edgeTriangleIndex = triangleIndex + 3 * gl_WorkGroupSize.x;
    // TODO: Remember to resize buffer to 2x size for this operation:
    const uint boundingEdgeTriangleIndex = gl_LocalInvocationID.x * 3 + (POINT_COUNT + hexID) * 3 * gl_WorkGroupSize.x * 2; // gl_WorkGroupSize.y == 2 in previous generation invocation

    if (!insideHull) {
        // Mark all this hex's vertices as invalid (rest of worker group will also do this)
        for (uint i = 0; i < 3; ++i)
            vertices[triangleIndex + i] = vec4(0.0);
        return;
    }

    const ivec2 neighbor = ivec2(col, row) + NEIGHBORS[gl_LocalInvocationID.x];
    const bool gridEdge = (neighbor.x < 0 || neighbor.y < 0 || num_cols <= neighbor.x || num_rows * 2 < neighbor.y);
    // If this workgroups hex's neighbour is outside the edge, it's definitively part of the contouring edge
//    if (gridEdge)
//        return;

    vec4 neighborPos = disp_mat * vec4(neighbor.x, neighbor.y, neighborDepth, 1.0);
    neighborPos /= neighborPos.w;

    // Only edge if this hex is inside hull but neighbour isn't
    if (isInsideHull(neighborPos))
        return;

    // Mark the edges generated from the triangle generation pass as invalid (they're being moved to the end)
    for (uint i = 0; i < 3; ++i)
        vertices[edgeTriangleIndex + i] = vec4(0.0);

    const float extrudeDepth = height + extrude_factor;

    float ar = HEX_ANGLE * float(gl_LocalInvocationID.x + 3);
    vertices[boundingEdgeTriangleIndex] = vec4(neighborPos.xy + vec2(tile_scale * cos(ar), tile_scale * sin(ar)), extrudeDepth, 1.0);

    for (uint i = 0; i < 2u; ++i) {
        float angle_rad = HEX_ANGLE * float(gl_LocalInvocationID.x + i);

        // Offset from center of point + grid width
        vec3 offset = vec3(tile_scale * cos(angle_rad), tile_scale * sin(angle_rad), 0.0);

        uint ti = boundingEdgeTriangleIndex + 2 - i;
        vertices[ti] = vec4(centerPos.xyz + offset, 1.0);
    }

    // TODO: Add other side of edge
}