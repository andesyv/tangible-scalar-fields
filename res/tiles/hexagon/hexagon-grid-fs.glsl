#version 450
#include "/defines.glsl"
#include "/globals.glsl"

layout (location = 0) out vec4 gridTexture;

uniform vec3 borderColor;
uniform float gridWidth;

//in screen space
in float tileSizeSS;
in vec2 tileCenterSS;

void main()
{

	float vertical_space = sqrt(3) * tileSizeSS;

 	vec2 leftBottomCorner = vec2(tileCenterSS + vec2(-tileSizeSS/2.0f, -vertical_space/2.0f));     
	vec2 leftCenterCorner = vec2(tileCenterSS + vec2(-tileSizeSS, 0.0f));
	vec2 leftTopCorner = vec2(tileCenterSS + vec2(-tileSizeSS/2.0f, vertical_space/2.0f));     
	vec2 rightBottomCorner = vec2(tileCenterSS + vec2(tileSizeSS/2.0f, -vertical_space/2.0f));     
	vec2 rightCenterCorner = vec2(tileCenterSS + vec2(tileSizeSS, 0.0f));
	vec2 rightTopCorner = vec2(tileCenterSS + vec2(tileSizeSS/2.0f, vertical_space/2.0f)); 

	//neighbours in vector are neghbours in geometry
	//vec4 border = vec4(xl,yb,xr,yt);
	//vec4 borderNorm = border / gridWidth;

	if(distance(rightCenterCorner, vec2(gl_FragCoord)) < 10){
		gridTexture = vec4(borderColor, 1);
	}

	/*
	if(border.y < gridWidth && 
		pointLeftOfLine(vec2(rightBottomCorner.x - gridWidth, rightBottomCorner.y + gridWidth), rightBottomCorner, vec2(gl_FragCoord)) &&
		pointLeftOfLine(leftBottomCorner, leftBottomCorner + gridWidth, vec2(gl_FragCoord)))
	{
		gridTexture = vec4(borderColor, 1-borderNorm.y);
	}
	if(border.z < gridWidth && 
		pointLeftOfLine(rightBottomCorner, vec2(rightBottomCorner.x - gridWidth, rightBottomCorner.y + gridWidth), vec2(gl_FragCoord)) &&
		pointLeftOfLine(rightTopCorner - gridWidth, rightTopCorner, vec2(gl_FragCoord)))
	{
		gridTexture = vec4(borderColor, 1-borderNorm.z);
	}
	if(border.w < gridWidth && 
		pointLeftOfLine(vec2(leftTopCorner.x + gridWidth, leftTopCorner.y - gridWidth), leftTopCorner, vec2(gl_FragCoord)) &&
		pointLeftOfLine(rightTopCorner, rightTopCorner - gridWidth, vec2(gl_FragCoord)))
	{
		gridTexture = vec4(borderColor, 1-borderNorm.w);
	}*/
}