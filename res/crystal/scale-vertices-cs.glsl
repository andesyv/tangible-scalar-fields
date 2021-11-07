#version 450

layout(local_size_x = 1) in;

layout(std430, binding = 0) buffer vertexBuffer
{
    vec4 vertices[];
};

uniform int triangleCount = 0;
uniform mat4 MVP = mat4(1.0);

void main() {
    const uint id =
    gl_WorkGroupID.x +
    gl_WorkGroupID.y * gl_NumWorkGroups.x +
    gl_WorkGroupID.z * gl_NumWorkGroups.x * gl_NumWorkGroups.y;
    // Early quit if this invocation is outside range
    if (triangleCount <= id)
        return;

    const uint triangleIndex = id * 3;
    if (vertices[triangleIndex].w < 1.0 || vertices[triangleIndex+1].w < 1.0 || vertices[triangleIndex+2].w < 1.0)
        return;

    for (int i = 0; i < 3; ++i)
        vertices[triangleIndex+i] = MVP * vec4(vertices[triangleIndex+i].xyz, 1.0);
}