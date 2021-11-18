#version 460
#extension GL_ARB_shading_language_include : required

// 6 triangles per hex, and every triangle (might) have a neighbour = 6 * 2
layout(local_size_x = 6, local_size_y = 2) in;

/**
 *  GeometryMode {
 *      Normal = 0,
 *      Mirror = 1,
 *      Cut = 2,
 *      Concave = 3
 *  };
 */

uniform int num_cols = 4;
uniform int num_rows = 4;
uniform float tile_scale = 0.6;
uniform mat4 disp_mat = mat4(1.0);
uniform uint POINT_COUNT = 1u;
uniform bool tileNormalsEnabled = false;
uniform int maxTexCoordY;
uniform float tileNormalDisplacementFactor = 1.0;
uniform int geometryMode = 0;
uniform bool alignNormalPlaneWithBottomMesh = true;
uniform float valueThreshold = 0.0;
uniform float cutValue = 0.5;
uniform float cutWidth = 0.1;
uniform float extrude_factor = 0.0;

layout(std430, binding = 0) buffer genVertexBuffer
{
    vec4 vertices[];
};
layout(binding = 1) uniform sampler2D accumulateTexture;
layout(std430, binding = 2) buffer valueMaxBuffer
{
    uint maxAccumulate;
    uint maxPointAlpha;
};
layout(std430, binding = 3) buffer tileNormalsBuffer
{
    int tileNormals[];
};
layout(binding = 4) uniform atomic_uint maxValDiff;

#include "/geometry-globals.glsl"

void main() {
    // Common for whole work group:
    /// Could make one local invocation do all of this common work for the whole group
    uint hexID =
        gl_WorkGroupID.x +
        gl_WorkGroupID.y * gl_NumWorkGroups.x +
        gl_WorkGroupID.z * gl_NumWorkGroups.x * gl_NumWorkGroups.y;
    // Early quit if this invocation is outside range
    if ((geometryMode != 0 ? 2 * POINT_COUNT : POINT_COUNT) <= hexID)
        return;

    const bool mirrorFlip = POINT_COUNT <= hexID;
    hexID = hexID % POINT_COUNT;

    //calculate position of hexagon center - in double height coordinates!
    //https://www.redblobgames.com/grids/hexagons/#coordinates-doubled
    const int col = int(hexID % num_cols);
    const int row = int(hexID/num_cols) * 2 - (col % 2 == 0 ? 0 : 1);

    // get value from accumulate texture
    ivec2 tilePosInAccTexture = ivec2(col, floor(hexID/num_cols));
    float hexValue = texelFetch(accumulateTexture, tilePosInAccTexture, 0).r;

    // hexagon-tiles-fs.glsl: 109
    const float maxAcc = uintBitsToFloat(maxAccumulate) + 1;
    const float cutMin = 2.0 * cutValue - 1.0 - cutWidth;
    const float cutMax = 2.0 * cutValue - 1.0 + cutWidth;

    vec3 normal = tileNormalsEnabled && EPSILON < hexValue ? getRegressionPlaneNormal(ivec2(col, row)) : vec3(0.0);

    float depth;
    switch (geometryMode) {
        case 1: // Mirror
            depth = (mirrorFlip ? -1.0 : 1.0) * hexValue / maxAcc;
            break;
        case 2: // Cut
            depth = mirrorFlip ? (hexValue < EPSILON ? cutMin : min(2.0 * hexValue / maxAcc - 1.0, cutMin)) : max(2.0 * hexValue / maxAcc - 1.0, cutMax);
            break;
        case 3: // Concave
            if (alignNormalPlaneWithBottomMesh && mirrorFlip) {
                /// There are definitively cheaper ways to do this using trigonometry, but I couldn't figure it out
                float normalDisplacement = dot(normalize(vec3(normal.xy, 0.0)) * tile_scale, normal) * normal.z;
                depth = hexValue / maxAcc + (!isnan(normalDisplacement) ? normalDisplacement * tileNormalDisplacementFactor : 0.0);
            } else
                depth = hexValue / maxAcc;
            break;
        default:
            depth = 2.0 * hexValue / maxAcc - 1.0; // [0,maxAccumulate] -> [-1, 1]
    }


    vec4 centerPos = disp_mat * vec4(col, row, depth, 1.0);
    centerPos /= centerPos.w;

    const uint bufferSize = POINT_COUNT * 6 * 2 * 3 * 2;

    // Individual for each work group:

    // Triangles are 3 vertices, and a hexagon is 6 triangles + 6 sides, so skip by 2*6*3 per hexagon.
    // Triangles are 3 vertices, so skip by 3 for each local invocation
    const uint triangleIndex = gl_LocalInvocationID.x * 3 + gl_LocalInvocationID.y * 3 * gl_WorkGroupSize.x + hexID * 3 * gl_WorkGroupSize.x * gl_WorkGroupSize.y + (mirrorFlip ? bufferSize : 0);
    const bool innerGroup = gl_LocalInvocationID.y == 0;

    // Just in case we're going to skip some triangles:
    vertices[triangleIndex] = vec4(0.0);
    vertices[triangleIndex+1] = vec4(0.0);
    vertices[triangleIndex+2] = vec4(0.0);

    // https://www.redblobgames.com/grids/hexagons/#neighbors-doubled
    const ivec2 neighbor = ivec2(col, row) + NEIGHBORS[gl_LocalInvocationID.x];
    const bool gridEdge = (neighbor.x < 0 || neighbor.y < -1 || num_cols <= neighbor.x || num_rows * 2 < neighbor.y);

    // Grid edge:
    //        if (gridEdge)
    //            return;

    // get value from accumulate texture
    // Reverse of: const float row = floor(hexID/num_cols) * 2.0 - (mod(col,2) == 0 ? 0.0 : 1.0);
    ivec2 neighborTilePosInAccTexture = ivec2(neighbor.x, (neighbor.y + (neighbor.x % 2 == 0 ? 0 : 1)) / 2);
    float neighborHexValue = gridEdge ? 0 : texelFetch(accumulateTexture, neighborTilePosInAccTexture, 0).r;

    vec3 neighborNormal = tileNormalsEnabled && EPSILON < neighborHexValue ? getRegressionPlaneNormal(neighbor) : vec3(0.0);

    if (geometryMode == 3) {
        uint neighborValueDiff = uint(hexValueIntMax * (abs(neighborHexValue - hexValue) / maxAcc));
        // Since we're running compute shaders, it would be possible to sync this across work groups before syncing it globally. May increase performance
        atomicCounterMax(maxValDiff, neighborValueDiff);
    }

    if (innerGroup) { // Hexagon triangles:
        vertices[triangleIndex] = centerPos;
    } else { // Neighbor triangles
        float neighborDepth;
        switch (geometryMode) {
            case 1: // Mirror
                neighborDepth = (mirrorFlip ? -1.0 : 1.0) * neighborHexValue / maxAcc;
                break;
            case 2: // Cut
                neighborDepth = mirrorFlip ? (neighborHexValue < EPSILON ? cutMin : min(2.0 * neighborHexValue / maxAcc - 1.0, cutMin)) : max(2.0 * neighborHexValue / maxAcc - 1.0, cutMax);
                break;
            case 3: // Concave
                if (alignNormalPlaneWithBottomMesh && mirrorFlip) {
                    float normalDisplacement = dot(normalize(vec3(neighborNormal.xy, 0.0)) * tile_scale, neighborNormal) * neighborNormal.z;
                    neighborDepth = neighborHexValue / maxAcc + (!isnan(normalDisplacement) ? normalDisplacement * tileNormalDisplacementFactor : 0.0);
                } else
                    neighborDepth = neighborHexValue / maxAcc;
                break;
            default:
                neighborDepth = 2.0 * neighborHexValue / maxAcc - 1.0;
        }

        // Skip "empty" triangles (can only skip early if we don't use normals)
        if (!tileNormalsEnabled && abs(depth - neighborDepth) < EPSILON)
            return;

        vec4 neighborPos = disp_mat * vec4(neighbor.x, neighbor.y, neighborDepth, 1.0);
        neighborPos /= neighborPos.w;

        vec3 neighborOffset = getOffset(3, neighborNormal);
        vertices[triangleIndex] = vec4(neighborPos.xyz + neighborOffset, 1.0);
    }

    for (uint i = 0; i < 2u; ++i) {
        vec3 offset = getOffset(i, normal);

        // Flip triangle winding order when on inner group and when flipping the mesh
        uint ti = triangleIndex + ((innerGroup ^^ mirrorFlip) ? (i + 1) : (2 - i));
        vertices[ti] = vec4(centerPos.xyz + offset, 1.0);
    }

    // Additional empty triangle check at the very end if tileNormals are enabled because we can't check for it early
    if (tileNormalsEnabled && !innerGroup) {
        float mi = 1000.0;
        float ma = -1000.0;
        for (int i = 0; i < 3; ++i) {
            float height = vertices[triangleIndex + i].z;
            mi = min(height, mi);
            ma = max(height, ma);
        }
        if (abs(mi - ma) < EPSILON)
            for (int i = 0; i < 3; ++i)
                vertices[triangleIndex + i].w = 0.0;
    }
}