#version 450
#extension GL_ARB_shading_language_include : require
#include "/defines.glsl"

layout(location = 0) in float xValue;

uniform float horizontal_space;
uniform float vertical_space;
uniform float hexSize;
uniform int num_cols;
uniform vec2 minBounds;

out VS_OUT {
    vec2 accTexPosition;
} vs_out;

void main()
{
	//calculate position of hexagon center
	float row;
	float col = mod(gl_VertexID,num_cols);

	if(mod(col,2) == 0){
		row = floor(gl_VertexID/num_cols) * 2;
		vs_out.accTexPosition = vec2(col, row);
	}
	else{
		row = (floor(gl_VertexID/num_cols) * 2) - 1;
		vs_out.accTexPosition = vec2(col, row + 1);
	}


	gl_Position =  vec4(col * horizontal_space + minBounds.x + hexSize/2.0f, row * vertical_space/2.0f + minBounds.y + vertical_space/2.0f, 0.0f, 1.0f);
}