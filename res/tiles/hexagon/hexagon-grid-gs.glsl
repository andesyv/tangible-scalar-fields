#version 450
#include "/defines.glsl"

#define N 6

layout(points) in;
layout(line_strip, max_vertices = N+1) out;

uniform sampler2D accumulateTexture;

uniform float tileSize;
uniform mat4 modelViewProjectionMatrix;

const float PI = 3.1415926;

in VS_OUT {
    vec2 accTexPosition;
} gs_in[];

void main()
{
	// get value from accumulate texture
    float hexValue = texelFetch(accumulateTexture, ivec2(gs_in[0].accTexPosition), 0).r;

    // we dont want to render the grid for empty hexs
    if(hexValue > 0){

		float angle_rad;
		vec4 offset;

		//Emit 6 vertices for hexagon
	    for (int i = 0; i <= N; i++) {
		
			//flat-topped
			float angle_deg = 60 * i;
   			angle_rad = PI / 180 * angle_deg;

			// Offset from center of point
        	offset = vec4(tileSize * cos(angle_rad), tileSize*sin(angle_rad), 0.0, 0.0);
			gl_Position = modelViewProjectionMatrix * (gl_in[0].gl_Position + offset);

			EmitVertex();
		}

		EndPrimitive();
	}
}