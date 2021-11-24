#version 450
#extension GL_ARB_shading_language_include : required
#include "/geometry-constants.glsl"

layout(local_size_x = 6, local_size_y = 1) in;

layout(std430, binding = 0) buffer vertexBuffer
{
    vec4 vertices[];
};
layout(binding = 4) uniform atomic_uint maxValDiff;
layout(std430, binding = 5) buffer notchGeometryBuffer
{
    bool notchModifiedGeometries[];
};

uniform float orientationNotchScale = 1.0;
uniform uint POINT_COUNT = 1u;
uniform bool mirroredMesh = true;
uniform bool concaveMesh = false;
uniform mat4 MVP = mat4(1.0);
uniform mat4 MVP2 = mat4(1.0);

mat4 translate(mat4 m, vec3 translation) {
    mat4 t = mat4(1.0);
    t[3].xyz = translation;
    return t * m;
}

mat4 getModelMatrix(bool mirrorFlip) {
    mat4 ModelMatrix = (mirrorFlip ? MVP2 : MVP);
    if (concaveMesh && mirrorFlip) {
        float scaleHeight = (ModelMatrix * vec4(0., 0., 1.0, 0.)).z;
        float extrude_amount = float(atomicCounter(maxValDiff) / double(hexValueIntMax));
        ModelMatrix = translate(ModelMatrix, vec3(0., 0., -extrude_amount * scaleHeight));
    }
    return ModelMatrix;
}

struct Line {
    vec3 dir;
    vec3 pos;
};

Line findOrientationPlaneIntersectingLine() {
    mat4 m1 = getModelMatrix(true);
    mat4 m2 = getModelMatrix(false);

    vec4 n1 = normalize(m1 * vec4(normalize((normalize(vec3(-1.0, -1.0, 0.0)) + vec3(0., 0.0, 1.0)) * 0.5), 0.0));
    vec4 n2 = normalize(m2 * vec4(normalize((normalize(vec3(-1.0, -1.0, 0.0)) + vec3(0., 0.0, -1.0)) * 0.5), 0.0));

//    n1 = m1 * n1;
//    n2 = m2 * n2;

    vec4 p1 = m1 * vec4(1.0, 1.0, orientationNotchScale, 1.0);
//    p1 /= p1.w;
    vec4 p2 = m2 * vec4(1.0, 1.0, -orientationNotchScale, 1.0);
//    p2 /= p2.w;

    float d1 = -dot(n1.xyz, p1.xyz);
    float d2 = -dot(n2.xyz, p2.xyz);

    Line l;
    l.dir = normalize(cross(n1.xyz, n2.xyz));

    float z = ((n2.y/n1.y)*d1 -d2)/(n2.z - n1.z*n2.y/n1.y);
    float y = (-n1.z * z -d1) / n1.y;
    l.pos = vec3(0., y, z);

    return l;
}

void main() {
    uint hexID =
    gl_WorkGroupID.x +
    gl_WorkGroupID.y * gl_NumWorkGroups.x +
    gl_WorkGroupID.z * gl_NumWorkGroups.x * gl_NumWorkGroups.y;
    // Early quit if this invocation is outside range
    if ((mirroredMesh ? 2 * POINT_COUNT : POINT_COUNT) <= hexID)
        return;


    const bool mirrorFlip = POINT_COUNT <= hexID;
    hexID = hexID % POINT_COUNT;
    const uint mirrorBufferOffset = POINT_COUNT * 6 * 2 * 3 * 2;

    const uint localID = gl_LocalInvocationID.x + hexID * gl_WorkGroupSize.x;
    if (!notchModifiedGeometries[localID])
        return;

    const uint triangleIndex = gl_LocalInvocationID.x * 3 + hexID * 6 * gl_WorkGroupSize.x + (mirrorFlip ? mirrorBufferOffset : 0);
    const uint boundingEdgeTriangleIndex = gl_LocalInvocationID.x * 6 + (POINT_COUNT + hexID) * 6 * gl_WorkGroupSize.x + (mirrorFlip ? mirrorBufferOffset : 0);
    const uint additionalGeometryIndex = gl_LocalInvocationID.x * 3 + (mirrorFlip ? POINT_COUNT : 0 + hexID) * 3 * gl_WorkGroupSize.x + 2 * mirrorBufferOffset;

    vec3 centerPoint = vertices[boundingEdgeTriangleIndex + 5].xyz;

    Line intersection = findOrientationPlaneIntersectingLine();
    // Project centerpoint onto line formed from the intersection of the planes
    vec3 lineToPoint = centerPoint - intersection.pos;
    vec3 lineNormal = normalize(intersection.dir);
    vec3 centerProjectedPoint = intersection.pos + lineNormal * dot(lineToPoint, lineNormal);

    vertices[additionalGeometryIndex] = vec4(centerProjectedPoint, 1.0);
    vertices[additionalGeometryIndex + 1] = vertices[boundingEdgeTriangleIndex + 5];
    vertices[additionalGeometryIndex + 2] = vertices[boundingEdgeTriangleIndex + 4];
}