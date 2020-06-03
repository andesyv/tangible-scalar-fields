#version 450
#include "/defines.glsl"
#include "/globals.glsl"

layout (location = 0) out vec4 gridTexture;

uniform vec3 borderColor;
uniform float gridWidth;

in vec2 isCoords;
in float tileSizeScreenSpace;
in vec2 tileCenterScreenSpace;

uniform int windowHeight;
uniform int windowWidth;

void main()
{

	// from the center to the edges the distance is half the tile size
	float tileSize_2 = tileSizeScreenSpace / 2.0f;

	vec2 leftBottomCorner = tileCenterScreenSpace - tileSize_2;
    vec2 leftTopCorner = vec2(tileCenterScreenSpace.x - tileSize_2, tileCenterScreenSpace.y + tileSize_2);
    vec2 rightBottomCorner = vec2(tileCenterScreenSpace.x + tileSize_2, tileCenterScreenSpace.y - tileSize_2);
    vec2 rightTopCorner = tileCenterScreenSpace + tileSize_2;

	float xr = (tileCenterScreenSpace.x + tileSize_2) - gl_FragCoord.x;
	float xl =  gl_FragCoord.x - (tileCenterScreenSpace.x - tileSize_2);
	float yt = (tileCenterScreenSpace.y + tileSize_2) - gl_FragCoord.y;
	float yb = gl_FragCoord.y - (tileCenterScreenSpace.y - tileSize_2);

	//neighbours in vector are neghbours in geometry
	vec4 border = vec4(xl,yb,xr,yt);
	vec4 borderNorm = border / gridWidth;

	if(border.x < gridWidth && 
		pointLeftOfLine(leftTopCorner, vec2(leftTopCorner.x + gridWidth, leftTopCorner.y - gridWidth), vec2(gl_FragCoord)) &&
		pointLeftOfLine(leftBottomCorner + gridWidth, leftBottomCorner, vec2(gl_FragCoord)))
	{
		gridTexture = vec4(borderColor, 1-borderNorm.x);
	}
	
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
	}
}