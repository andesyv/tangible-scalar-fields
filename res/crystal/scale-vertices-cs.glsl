#version 450

layout(local_size_x = 1) in;

layout(std430, binding = 0) buffer vertexBuffer
{
    vec4 vertices[];
};

layout(binding = 4) uniform atomic_uint maxValDiff;

uniform int triangleCount = 0;
uniform bool mirroredMesh = false;
uniform bool concaveMesh = false;
uniform mat4 MVP = mat4(1.0);
uniform mat4 MVP2 = mat4(1.0);

const uint hexValueIntMax = 1000000; // The higher the number, the more accurate (max 32-bit uint)

mat4 translate(mat4 m, vec3 translation) {
    mat4 t = mat4(1.0);
    t[3].xyz = translation;
    return t * m;
}

void main() {
    const uint id =
    gl_WorkGroupID.x +
    gl_WorkGroupID.y * gl_NumWorkGroups.x +
    gl_WorkGroupID.z * gl_NumWorkGroups.x * gl_NumWorkGroups.y;
    // Early quit if this invocation is outside range
    if ((mirroredMesh ? 2 * triangleCount : triangleCount) <= id)
        return;

    const bool mirrorFlip = triangleCount <= id;

    const uint triangleIndex = id * 3;
    if (vertices[triangleIndex].w < 1.0 || vertices[triangleIndex+1].w < 1.0 || vertices[triangleIndex+2].w < 1.0)
        return;

    mat4 ModelMatrix = (mirrorFlip ? MVP2 : MVP);
    if (concaveMesh && mirrorFlip) {
        float scaleHeight = (ModelMatrix * vec4(0., 0., 1.0, 0.)).z;
        float extrude_amount = float(atomicCounter(maxValDiff) / double(hexValueIntMax));
        ModelMatrix = translate(ModelMatrix, vec3(0., 0., -extrude_amount * scaleHeight));
    }

    for (int i = 0; i < 3; ++i)
        vertices[triangleIndex+i] = ModelMatrix * vec4(vertices[triangleIndex+i].xyz, 1.0);
}