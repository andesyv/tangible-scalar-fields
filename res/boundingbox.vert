#version 330

in vec3 a_pos;
uniform mat4 u_modelViewProjection;

void main()
{
	gl_Position = u_modelViewProjection * vec4(a_pos,1.0);
}
