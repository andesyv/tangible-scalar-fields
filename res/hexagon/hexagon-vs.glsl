#version 450
#extension GL_ARB_shading_language_include : require
#include "/defines.glsl"

layout(location = 0) in float xValue;

uniform float horizontal_space;
uniform float vertical_space;
uniform int num_cols;
uniform vec2 data_offset;

void main()
{
	//calculate position of hexagon center
	float row;
	float col = mod(gl_VertexID,num_cols);

	if(mod(col,2) == 0){
		row = floor(gl_VertexID/num_cols) * 2;
	}
	else{
		row = (floor(gl_VertexID/num_cols) * 2) + 1;
	}

	gl_Position =  vec4(col * horizontal_space + data_offset.x, row * vertical_space + data_offset.y, 0.0f, 1.0f);
}