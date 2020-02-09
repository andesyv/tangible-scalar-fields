#version 450
#extension GL_ARB_shading_language_include : require
#include "/defines.glsl"

layout(location = 0) in float xValue;

uniform float horizontal_space;
uniform float vertical_space;
uniform int num_cols;

void main()
{
	float col;
	float row = mod(gl_VertexID,num_cols);

	if(mod(row,2) == 0){
		col = floor(gl_VertexID/num_cols) * 2;
	}
	else{
		col = (floor(gl_VertexID/num_cols) * 2) + 1;
	}

	gl_Position = vec4(col * horizontal_space, row * vertical_space, 0.0f, 1.0f);
}