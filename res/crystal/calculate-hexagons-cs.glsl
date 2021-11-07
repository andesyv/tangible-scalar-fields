#version 450

// 6 triangles per hex, and every triangle (might) have a neighbour = 6 * 2
layout(local_size_x = 6, local_size_y = 2) in;

layout(std430, binding = 0) buffer genVertexBuffer
{
    vec4 vertices[];
};

uniform int num_cols = 4;
uniform int num_rows = 4;
uniform float tile_scale = 0.6;
uniform mat4 disp_mat = mat4(1.0);
uniform uint POINT_COUNT = 1u;
uniform bool tileNormalsEnabled = false;
uniform int maxTexCoordY;
uniform float tileNormalDisplacementFactor = 1.0;
uniform bool mirrorMesh = false;
uniform bool mirrorFlip = false;

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


const float PI = 3.1415926;
const float HEX_ANGLE = PI / 180.0 * 60.0;
const float bufferAccumulationFactor = 100.0; // Uniform constant in hexagon-tile shaders (could be a define)
const float EPSILON = 0.001;

const ivec2 NEIGHBORS[6] = ivec2[6](
    ivec2(1, 1), ivec2(0, 2), ivec2(-1, 1),
    ivec2(-1,-1), ivec2(0, -2), ivec2(1, -1)
);

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
    const float maxAcc = uintBitsToFloat(maxAccumulate) + 1;

    float depth;
    if (mirrorMesh) {
        depth = (mirrorFlip ? -1.0 : 1.0) * hexValue / maxAcc;
    } else {
        depth = 2.0 * hexValue / maxAcc - 1.0; // [0,maxAccumulate] -> [-1, 1]
    }

    vec4 centerPos = disp_mat * vec4(col, row, depth, 1.0);
    centerPos /= centerPos.w;

    vec3 normal = tileNormalsEnabled ? getRegressionPlaneNormal(ivec2(col, row)) : vec3(0.0);
//    centerPos.xyz += 0.1 * normal;


    // Individual for each work group:

    // Triangles are 3 vertices, and a hexagon is 6 triangles + 6 sides, so skip by 2*6*3 per hexagon.
    // Triangles are 3 vertices, so skip by 3 for each local invocation
    const uint triangleIndex = gl_LocalInvocationID.x * 3 + gl_LocalInvocationID.y * 3 * gl_WorkGroupSize.x + hexID * 3 * gl_WorkGroupSize.x * gl_WorkGroupSize.y;
    const bool innerGroup = gl_LocalInvocationID.y == 0;

    // Just in case we're going to skip some triangles:
    vertices[triangleIndex] = vec4(0.0);
    vertices[triangleIndex+1] = vec4(0.0);
    vertices[triangleIndex+2] = vec4(0.0);

    if (innerGroup) { // Hexagon triangles:
        vertices[triangleIndex] = centerPos;
    } else { // Neighbor triangles
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

        float neighborDepth;
        if (mirrorMesh) {
            neighborDepth = (mirrorFlip ? -1.0 : 1.0) * neighborHexValue / maxAcc;
        } else {
            neighborDepth = 2.0 * neighborHexValue / maxAcc - 1.0;
        }

        // Skip "empty" triangles (can only skip early if we don't use normals)
        if (!tileNormalsEnabled && abs(depth - neighborDepth) < EPSILON)
            return;

        vec4 neighborPos = disp_mat * vec4(neighbor.x, neighbor.y, neighborDepth, 1.0);
        neighborPos /= neighborPos.w;

        float ar = HEX_ANGLE * float(gl_LocalInvocationID.x + 3);
        vec3 neighborOffset = vec3(tile_scale * cos(ar), tile_scale * sin(ar), 0.);
        // Regression plane offset
        if (tileNormalsEnabled) {
            vec3 neighborNormal = tileNormalsEnabled ? getRegressionPlaneNormal(neighbor) : vec3(0.0);
            float normalDisplacement = dot(-neighborOffset, neighborNormal) * neighborNormal.z;

            if (!isnan(normalDisplacement))
                neighborOffset.z += normalDisplacement * tileNormalDisplacementFactor;
        }
        vertices[triangleIndex] = vec4(neighborPos.xyz + neighborOffset, 1.0);
    }

    for (uint i = 0; i < 2u; ++i) {
        float angle_rad = HEX_ANGLE * float(gl_LocalInvocationID.x + i);

        // Offset from center of point + grid width
        vec3 offset = vec3(tile_scale * cos(angle_rad), tile_scale * sin(angle_rad), 0.0);
        if (tileNormalsEnabled) {
            float normalDisplacement = dot(-offset, normal) * normal.z;
            if (!isnan(normalDisplacement))
                offset.z += normalDisplacement * tileNormalDisplacementFactor;
        }

        // Flip triangle winding order when on inner group and when flipping the mesh
        uint ti = triangleIndex + ((innerGroup ^^ mirrorFlip) ? (i + 1) : (2 - i));
        vertices[ti] = vec4(centerPos.xyz + offset, 1.0);
    }

    // Additional empty triangle check at the very end if tileNormals are enabled because we can't check for it early
    if (!innerGroup && tileNormalsEnabled) {
        float mi = 1000.0;
        float ma = 1000.0;
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