#version 450

#define TESSELATION 4

layout(vertices = 4) out;

in vec2 vTexCoord[];
out vec2 tTexCoord[];

void main()
{
    // ----------------------------------------------------------------------
    // pass attributes through
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    tTexCoord[gl_InvocationID] = vTexCoord[gl_InvocationID];

    // ----------------------------------------------------------------------
    // invocation zero controls tessellation levels for the entire patch
    if (gl_InvocationID == 0)
    {
        gl_TessLevelOuter[0] = TESSELATION;
        gl_TessLevelOuter[1] = TESSELATION;
        gl_TessLevelOuter[2] = TESSELATION;
        gl_TessLevelOuter[3] = TESSELATION;

        gl_TessLevelInner[0] = TESSELATION;
        gl_TessLevelInner[1] = TESSELATION;
    }
}