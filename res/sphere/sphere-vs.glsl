#version 400

in vec3 position;
out int vertexId;

void main()
{
    gl_Position = vec4(position.xyz,1.0);
	vertexId = gl_VertexID;
}