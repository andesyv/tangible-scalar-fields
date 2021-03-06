#version 450
#extension GL_ARB_shading_language_include : required
#include "/geometry-constants.glsl"

/**
 * Modifies the geometry with an orientation notch, which projects the vertices of the mesh onto 2 opposing planes,
 * creating a "split".
 */

layout(local_size_x = 6, local_size_y = 1) in;

layout(std430, binding = 0) buffer vertexBuffer
{
    vec4 vertices[];
};

uniform float notchDepth = 1.0;
uniform float notchHeightAdjust = 0.0;
uniform vec3 n1 = normalize((normalize(vec3(-1.0, -1.0, 0.0)) + vec3(0., 0.0, 1.0)) * 0.5);
uniform vec3 n2 = normalize((normalize(vec3(-1.0, -1.0, 0.0)) + vec3(0., 0.0, -1.0)) * 0.5);
uniform uint POINT_COUNT = 1u;
uniform bool mirroredMesh = true;

/**
 * Projects a point onto the middle plane of two planes. Then, if the point is within
 * the two planes, the z-axis of the projected point is returned
 */
float orientationPlaneProjectedDepth(vec3 pos, bool mirrored, out float middleLine) {
    const vec3 p1 = vec3(1.0, 1.0, notchHeightAdjust + notchDepth);
    const vec3 p2 = vec3(1.0, 1.0, notchHeightAdjust - notchDepth);

    // Middle plane between the two planes
    const vec3 n3 = vec3(0., 0., 1.0);
    const vec3 p3 = (p1 + p2) * 0.5;

    /* 
     * Projected point in z dir:
     * p' = p + dot(p3 - p, n3) / dot(n3, vec3(0., 0., 1.))
     * p' = p + dot(p3 - p, n3) / n3.z
     * p' = p + dot(p3 - p, n3)
     */
    vec3 middleProjected = pos + vec3(0., 0., dot(p3 - pos, n3));

    middleLine = middleProjected.z;

    float d1 = dot(p1 - middleProjected, n1);
    float d2 = dot(p2 - middleProjected, n2);

    // If both distances >= 0 then the points is within the planes
    if (-EPSILON < d1 && -EPSILON < d2)
        return mirrored ? middleProjected.z - d2 : middleProjected.z + d1;
    // Otherwise clamp to the middle plane
    else
        return middleProjected.z;
}

struct Line {
    vec3 dir;
    vec3 pos;
};

Line findOrientationPlaneIntersectingLine() {
    vec3 p1 = vec3(1.0, 1.0, notchHeightAdjust + notchDepth);
    vec3 p2 = vec3(1.0, 1.0, notchHeightAdjust - notchDepth);

    float d1 = -dot(n1, p1);
    float d2 = -dot(n2, p2);

    Line l;
    l.dir = normalize(cross(n1, n2));

    /**
     * We need to find any point the line travels through, which we do by solving the two equations:
     * n1.x * x + n1.y * y + n1.z * z + d1 = 0
     * n2.x * x + n2.y * y + n2.z * z + d2 = 0
     * As there's 3 variables in 2 equations, one variable is free. So by setting x = t we can get a solution
     * for all points given a t parameter. We don't care about all points, just one. So we set t = 0 so simplify.
     */
    float z = ((n2.y/n1.y)*d1 -d2)/(n2.z - n1.z*n2.y/n1.y);
    float y = (-n1.z * z -d1) / n1.y;
    l.pos = vec3(0., y, z);

    return l;
}

void main() {
    /// ------------------- Common for whole work group -------------------------------------------
    uint hexID =
    gl_WorkGroupID.x +
    gl_WorkGroupID.y * gl_NumWorkGroups.x +
    gl_WorkGroupID.z * gl_NumWorkGroups.x * gl_NumWorkGroups.y;
    // Early quit if this invocation is outside range
    if ((mirroredMesh ? 2 * POINT_COUNT : POINT_COUNT) <= hexID)
        return;

    const uint vertexCount = getVertexCount(POINT_COUNT);
    const bool mirrorFlip = POINT_COUNT <= hexID;
    hexID = hexID % POINT_COUNT;


    /// ------------------- Individual for each work group -------------------------------------------
    const uint triangleIndex = gl_LocalInvocationID.x * 3 + hexID * 6 * gl_WorkGroupSize.x + (mirrorFlip ? vertexCount : 0);
    const uint boundingEdgeTriangleIndex = gl_LocalInvocationID.x * 6 + hexID * 6 * gl_WorkGroupSize.x + (mirroredMesh ? (mirrorFlip ? 2 * vertexCount : vertexCount) : 0) + vertexCount;
    const uint additionalGeometryIndex = boundingEdgeTriangleIndex + (mirroredMesh ? vertexCount * 2 : vertexCount);

    bool geometryModified = false;
    float middleLine = 0.0;
    const uint edgeIndices[3] = uint[3](0, 4, 5);
    for (uint i = 0; i < 3; ++i) {
        vec4 pos = vertices[boundingEdgeTriangleIndex + edgeIndices[i]];
        // Project point onto split-plane
        pos.z = orientationPlaneProjectedDepth(pos.xyz, mirrorFlip, middleLine);
        if (EPSILON < abs(pos.z - middleLine))
            geometryModified = true;
        vertices[boundingEdgeTriangleIndex + edgeIndices[i]] = pos;
    }

    // If geometry is untouched, quit early
    if (!geometryModified)
        return;

    // If geometry was changed, add new geometry to fill up the "split" we made in the geometry.
    vec3 rightCenterPoint = vertices[boundingEdgeTriangleIndex].xyz;
    vec3 leftCenterPoint = vertices[boundingEdgeTriangleIndex+3].xyz;

    Line intersection = findOrientationPlaneIntersectingLine();
    vec3 lineNormal = normalize(intersection.dir);
    // Project centerpoint onto line formed from the intersection of the planes
    vec3 rightProjectedPoint = intersection.pos + lineNormal * dot(rightCenterPoint - intersection.pos, lineNormal);
    vec3 leftProjectedPoint = intersection.pos + lineNormal * dot(leftCenterPoint - intersection.pos, lineNormal);

    // First triangle:
    vertices[additionalGeometryIndex + 0] = vec4(rightProjectedPoint, 1.0);
    vertices[additionalGeometryIndex + 1] = vertices[boundingEdgeTriangleIndex + 5];
    vertices[additionalGeometryIndex + 2] = vertices[boundingEdgeTriangleIndex + 4];

    // Second triangle:
    vertices[additionalGeometryIndex + (mirrorFlip ? 5 : 3)] = vertices[boundingEdgeTriangleIndex + (mirrorFlip ? 5 : 4)];
    vertices[additionalGeometryIndex + 4] = vec4(leftProjectedPoint, 1.0);
    vertices[additionalGeometryIndex + (mirrorFlip ? 3 : 5)] = vec4(rightProjectedPoint, 1.0);
}