#version 450

layout(vertices = 4) out;

uniform uint tesselation = 16;

in vec3 vPos[];
in vec2 vTexCoord[];
out vec3 tPos[];
out vec2 tTexCoord[];

void main()
{
    // ----------------------------------------------------------------------
    // pass attributes through
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    tTexCoord[gl_InvocationID] = vTexCoord[gl_InvocationID];
    tPos[gl_InvocationID] = vPos[gl_InvocationID];

    // ----------------------------------------------------------------------
    // invocation zero controls tessellation levels for the entire patch
    if (gl_InvocationID == 0)
    {
        gl_TessLevelOuter[0] = tesselation;
        gl_TessLevelOuter[1] = tesselation;
        gl_TessLevelOuter[2] = tesselation;
        gl_TessLevelOuter[3] = tesselation;

        gl_TessLevelInner[0] = tesselation;
        gl_TessLevelInner[1] = tesselation;
    }
}