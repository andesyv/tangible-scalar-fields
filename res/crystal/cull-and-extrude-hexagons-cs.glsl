#version 450

layout(local_size_x = 6, local_size_y = 1) in;

layout(std430, binding = 0) buffer vertexBuffer
{
    vec4 vertices[];
};

uniform int num_cols = 4;
uniform int num_rows = 4;
uniform float tile_scale = 0.6;
uniform float extrude_factor = 0.5;
uniform mat4 disp_mat = mat4(1.0);
uniform uint POINT_COUNT = 1u;
uniform uint HULL_SIZE = 0u;
uniform bool tileNormalsEnabled = false;
uniform int maxTexCoordY;
uniform float tileNormalDisplacementFactor = 1.0;
uniform bool mirrorMesh = false;
uniform bool cutMesh = false;
uniform bool concaveMesh = false;
uniform float cutValue = 0.5;

layout(std430, binding = 1) buffer hullBuffer
{
//    uint convex_hull[]; // Should indicate which hexID's are part of the hull.
    vec4 convex_hull[];
};

layout(binding = 2) uniform sampler2D accumulateTexture;

layout(std430, binding = 3) buffer tileNormalsBuffer
{
    int tileNormals[];
};

layout(binding = 4) uniform atomic_uint maxValDiff;

const float PI = 3.1415926;
const float HEX_ANGLE = PI / 180.0 * 60.0;
const float bufferAccumulationFactor = 100.0; // Uniform constant in hexagon-tile shaders (could be a define)
const float EPSILON = 0.001;
const uint hexValueIntMax = 1000000; // The higher the number, the more accurate (max 32-bit uint)

const ivec2 NEIGHBORS[6] = ivec2[6](
    ivec2(1, 1), ivec2(0, 2), ivec2(-1, 1),
    ivec2(-1,-1), ivec2(0, -2), ivec2(1, -1)
);

vec4 getHexCenterPos(uint i) {
    // triangle size (3) * triangles (6) * stride (2 hexagons) = 3 * 6 * 2
    return vertices[3 * 6 * 2 * i];
}

float scalarCross2D(vec2 a, vec2 b) {
    vec3 c = cross(vec3(a, 0.0), vec3(b, 0.0));
    return c.z;
}

bool ccw(vec2 a, vec2 b) {
    return EPSILON <= scalarCross2D(a,b);
}

bool isInsideHull(vec4 pos) {
    for (uint i = 0; i < HULL_SIZE; ++i) {
        uint j = (i + 1) % HULL_SIZE;
        vec2 a = (convex_hull[j] - convex_hull[i]).xy;
        vec2 b = (pos - convex_hull[j]).xy;

        if (ccw(a,b))
            return false;
    }
    return true;
}

vec3 getRegressionPlaneNormal(ivec2 hexCoord) {
    vec4 tileNormal;
    // get accumulated tile normals from buffer
    for(int i = 0; i < 4; i++){
        tileNormal[i] = float(tileNormals[int((hexCoord.x*(maxTexCoordY+1) + hexCoord.y) * 5 + i)]);
    }
    tileNormal /= bufferAccumulationFactor;// get original value after accumulation

    // LIGHTING NORMAL ------------------------
    return normalize(vec3(tileNormal.x, tileNormal.y, tileNormal.w));
}

void main() {
    // Common for whole work group:
    /// Could make one local invocation do all of this common work for the whole group
    uint hexID =
    gl_WorkGroupID.x +
    gl_WorkGroupID.y * gl_NumWorkGroups.x +
    gl_WorkGroupID.z * gl_NumWorkGroups.x * gl_NumWorkGroups.y;
    // Early quit if this invocation is outside range
    if ((mirrorMesh || cutMesh || concaveMesh ? 2 * POINT_COUNT : POINT_COUNT) <= hexID)
        return;

    const bool mirrorFlip = POINT_COUNT <= hexID;
    hexID = hexID % POINT_COUNT;

    const uint bufferSize = POINT_COUNT * 6 * 2 * 3 * 2;

    //calculate position of hexagon center - in double height coordinates!
    //https://www.redblobgames.com/grids/hexagons/#coordinates-doubled
    const int col = int(hexID % num_cols);
    const int row = int(hexID/num_cols) * 2 - (col % 2 == 0 ? 0 : 1);

    ivec2 tilePosInAccTexture = ivec2(col, floor(hexID/num_cols));
    float hexValue = texelFetch(accumulateTexture, tilePosInAccTexture, 0).r;

    // vec4 centerPos = disp_mat * vec4(col, row, depth, 1.0);
    //    centerPos /= centerPos.w;

    const uint centerIndex = hexID * 3 * gl_WorkGroupSize.x * 2 + (mirrorFlip ? bufferSize : 0);
    vec4 centerPos = vertices[centerIndex];
    bool insideHull = isInsideHull(centerPos);

    vec3 normal = tileNormalsEnabled && EPSILON < hexValue ? getRegressionPlaneNormal(ivec2(col, row)) : vec3(0.0);

    // Individual for each work group:

    // Triangles are 3 vertices, and a hexagon is 6 triangles + 6 sides, so skip by 2*6*3 per hexagon.
    // Triangles are 3 vertices, so skip by 3 for each local invocation
    const uint triangleIndex = gl_LocalInvocationID.x * 3 + hexID * 3 * gl_WorkGroupSize.x * 2 + (mirrorFlip ? bufferSize : 0); // gl_WorkGroupSize.y == 2 in previous generation invocation
    const uint edgeTriangleIndex = triangleIndex + 3 * gl_WorkGroupSize.x;
    const uint boundingEdgeTriangleIndex = gl_LocalInvocationID.x * 6 + (POINT_COUNT + hexID) * 6 * gl_WorkGroupSize.x + (mirrorFlip ? bufferSize : 0); // gl_WorkGroupSize.y == 2 in previous generation invocation

    if (!insideHull) {
        // Mark all this hex's vertices as invalid (rest of worker group will also do this)
        for (uint i = 0; i < 3; ++i)
            vertices[triangleIndex + i].w = 0.0; // Don't wan't to mark xyz = 0, because neighbor might sample this one out of order

        for (uint i = 0; i < 3; ++i)
            vertices[edgeTriangleIndex + i] = vec4(0.0);
    }

    const ivec2 neighbor = ivec2(col, row) + NEIGHBORS[gl_LocalInvocationID.x];
    const bool gridEdge = neighbor.x < 0 || neighbor.y < -1 || num_cols <= neighbor.x || num_rows * 2 < neighbor.y;

    vec4 neighborPos = disp_mat * vec4(neighbor.x, neighbor.y, 0.0, 1.0);
    neighborPos /= neighborPos.w;


    // If this workgroups hex's neighbour is outside the edge, it's definitively part of the contouring edge
    const bool neighborInsideHull = gridEdge ? false : isInsideHull(neighborPos);
    // Only edge if this hex is inside hull but neighbour isn't, OR if the neighbour is but this isn't
    // https://gamedev.stackexchange.com/questions/87396/how-to-draw-the-contour-of-a-hexagon-area-like-in-civ-5
    const bool isEdge = bool(int(insideHull) ^ int(neighborInsideHull));
    if (!isEdge)
        return;

    const bool isNeighbor = neighborInsideHull;

    // Mark the edges generated from the triangle generation pass as invalid (they're being moved to the end)
    if (insideHull) {
        for (uint i = 0; i < 3; ++i)
            vertices[edgeTriangleIndex + i] = vec4(0.0);
    }

    // Doesn't work to do edge from neighbor side as neighbor might be outside the grid (never get's run)
    if (isNeighbor)
        return;

    float extrudeDepth;
    if (mirrorMesh)
        extrudeDepth = (mirrorFlip ? 0.5 : -0.5) * extrude_factor;
    else if (cutMesh)
        extrudeDepth = 2.0 * cutValue - 1.0; // (mirrorFlip ? 0.5 : -0.5) * extrude_factor;
    else if (concaveMesh) {
        float extra_extrude_amount = float(atomicCounter(maxValDiff) / double(hexValueIntMax));
        extrudeDepth = (mirrorFlip ? 0.5 : -0.5) * (extrude_factor + extra_extrude_amount);
    } else
        extrudeDepth = -extrude_factor - 1.0;

    float ar = HEX_ANGLE * float(gl_LocalInvocationID.x + 3);
    vertices[boundingEdgeTriangleIndex] = vec4(neighborPos.xy + vec2(tile_scale * cos(ar), tile_scale * sin(ar)), extrudeDepth, 1.0);

    for (uint i = 0; i < 2u; ++i) {
        float angle_rad = HEX_ANGLE * float(gl_LocalInvocationID.x + i);

        // Offset from center of point + grid width
        vec3 offset = vec3(tile_scale * cos(angle_rad), tile_scale * sin(angle_rad), 0.0);
        if (tileNormalsEnabled) {
            float normalDisplacement = dot(-offset, normal) * normal.z;
            if (!isnan(normalDisplacement))
                offset.z += normalDisplacement * tileNormalDisplacementFactor;
        }

        uint ti = boundingEdgeTriangleIndex + (mirrorFlip ? 1 + i : 2 - i);
        vertices[ti] = vec4(centerPos.xyz + offset, 1.0);
    }

    float angle_rad = HEX_ANGLE * float(gl_LocalInvocationID.x);

    // Offset from center of point + grid width
    vec3 offset = vec3(tile_scale * cos(angle_rad), tile_scale * sin(angle_rad), 0.0);

    if (tileNormalsEnabled) {
        float normalDisplacement = dot(-offset, normal) * normal.z;
        if (!isnan(normalDisplacement))
        offset.z += normalDisplacement * tileNormalDisplacementFactor;
    }

    vertices[boundingEdgeTriangleIndex+3] = vec4(centerPos.xyz + offset, 1.0);

    for (uint i = 0; i < 2u; ++i) {
        angle_rad = HEX_ANGLE * float(gl_LocalInvocationID.x + (mirrorFlip ? 4 - i : 3 + i));
        vertices[boundingEdgeTriangleIndex + 5 - i] = vec4(neighborPos.xy + vec2(tile_scale * cos(angle_rad), tile_scale * sin(angle_rad)), extrudeDepth, 1.0);
    }
}