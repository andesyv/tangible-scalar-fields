#version 450

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec3 fragPos[3];

uniform mat4 inverseMVP = mat4(1.0);

out vec3 normal;
out vec3 gFragPos;

void main()
{
    vec3 tangent = normalize(fragPos[2] - fragPos[1]);
    vec3 bitangent = normalize(fragPos[0] - fragPos[1]);

    vec3 norm = cross(tangent, bitangent);

    for (int i = 0; i < 3; ++i) {
        gl_Position = gl_in[i].gl_Position;
        normal = norm;
        gFragPos = fragPos[i];
        EmitVertex();
    }
    EndPrimitive();
}