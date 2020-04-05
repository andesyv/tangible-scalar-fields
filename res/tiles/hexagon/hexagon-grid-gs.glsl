#version 450
#include "/defines.glsl"

#define N 6

layout(points) in;
layout(line_strip, max_vertices = N+1) out;

uniform float hexSize;
uniform mat4 modelViewProjectionMatrix;

const float PI = 3.1415926;

void main()
{
		float angle_rad;
		vec4 offset;

		//Emit 6 vertices for hexagon
	    for (int i = 0; i <= N; i++) {
		
			//flat-topped
			float angle_deg = 60 * i;
   			float angle_rad = PI / 180 * angle_deg;

			// Offset from center of point
        	offset = vec4(hexSize * cos(angle_rad), hexSize*sin(angle_rad), 0.0, 0.0);
			gl_Position = modelViewProjectionMatrix * (gl_in[0].gl_Position + offset);

			EmitVertex();
		}

		EndPrimitive();
}