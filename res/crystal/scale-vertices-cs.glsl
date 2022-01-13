#version 450
#extension GL_ARB_shading_language_include : required
#include "/geometry-constants.glsl"

/**
 * 1 to 1 applies a transformation matrix to the vertices.
 * During geometry calculation we operate in the same space (to simplify operations). This shader applies a matrix
 * to the upper and lower part of the mesh to split them apart into world space coordinates.
 */

layout(local_size_x = 1) in;

layout(std430, binding = 0) buffer vertexBuffer
{
    vec4 vertices[];
};

layout(binding = 4) uniform atomic_uint maxValDiff;

uniform int mainTrianglesCount = 0;
uniform bool mirroredMesh = false;
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
    /*
     * For concave geometry, use the previously calculated difference between hexagons
     * to modify the modelmatrix with an additional translation such that the mesh doesn't self-intersect.
     */
    if (concaveMesh && mirrorFlip) {
        float scaleHeight = (ModelMatrix * vec4(0., 0., 1.0, 0.)).z;
        float extrude_amount = float(atomicCounter(maxValDiff) / double(hexValueIntMax));
        ModelMatrix = translate(ModelMatrix, vec3(0., 0., -extrude_amount * scaleHeight));
    }
    return ModelMatrix;
}

void main() {
    const uint id =
    gl_WorkGroupID.x +
    gl_WorkGroupID.y * gl_NumWorkGroups.x +
    gl_WorkGroupID.z * gl_NumWorkGroups.x * gl_NumWorkGroups.y;
    // Early quit if this invocation is outside range
    if ((mirroredMesh ? 4 : 2) * mainTrianglesCount <= id)
        return;

    const bool mirrorFlip = mirroredMesh && (id / mainTrianglesCount % 2 == 1);
    const uint triangleIndex = id * 3;
    if (vertices[triangleIndex].w < 1.0 || vertices[triangleIndex+1].w < 1.0 || vertices[triangleIndex+2].w < 1.0)
        return;

    mat4 ModelMatrix = getModelMatrix(mirrorFlip);

    for (int i = 0; i < 3; ++i)
        vertices[triangleIndex+i] = ModelMatrix * vec4(vertices[triangleIndex+i].xyz, 1.0);
}