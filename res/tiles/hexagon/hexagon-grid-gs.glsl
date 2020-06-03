#version 450
#include "/defines.glsl"
#include "/globals.glsl"

layout(points) in;
layout(triangle_strip, max_vertices = 10) out;

uniform sampler2D accumulateTexture;

uniform int windowHeight;
uniform int windowWidth;

uniform float tileSize;
uniform mat4 modelViewProjectionMatrix;

const float PI = 3.1415926;

in VS_OUT {
    vec2 accTexPosition;
} gs_in[];

//in screen space
out float tileSizeSS;
out vec2 tileCenterSS;

// i = 0 => right Center
// continue agains clock circle (i = 1 = right Top)
void emitHexagonVertex(int i){
	//flat-topped
	float angle_deg = 60 * i;
	float angle_rad = PI / 180 * angle_deg;

	// Offset from center of point
	vec4 offset = vec4(tileSize * cos(angle_rad), tileSize * sin(angle_rad), 0.0, 0.0);
	gl_Position = modelViewProjectionMatrix * (gl_in[0].gl_Position + offset);

	EmitVertex();
}

void emitHexagonCenterVertex(){
	gl_Position = modelViewProjectionMatrix * gl_in[0].gl_Position;
	EmitVertex();
}

void main()
{
	// get value from accumulate texture
    float hexValue = texelFetch(accumulateTexture, ivec2(gs_in[0].accTexPosition), 0).r;

    // we dont want to render the grid for empty hexs
    if(hexValue > 0){

		tileSizeSS = getScreenSpaceSize(modelViewProjectionMatrix, vec2(tileSize, 0.0f), windowWidth, windowHeight).x;
        tileCenterSS = getScreenSpacePosOfPoint(modelViewProjectionMatrix, vec2(gl_in[0].gl_Position), windowWidth, windowHeight);

		//Emit 10 vertices for triangulated hexagon
		emitHexagonVertex(0);
		emitHexagonCenterVertex();
		emitHexagonVertex(1);
		emitHexagonVertex(2);
		emitHexagonCenterVertex();
		emitHexagonVertex(3);
		emitHexagonVertex(4);
		emitHexagonCenterVertex();
		emitHexagonVertex(5);
		emitHexagonVertex(6);

		EndPrimitive();
	}
}