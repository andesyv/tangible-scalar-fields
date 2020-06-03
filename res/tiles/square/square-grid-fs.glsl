#version 450
#include "/defines.glsl"
#include "/globals.glsl"

layout (location = 0) out vec4 gridTexture;

uniform vec3 borderColor;
uniform float gridWidth;

in float tileSizeSS;
in vec2 tileCenterSS;
//left,bottom,right,top
in vec4 neighbourValue;

void main()
{

	// from the center to the tile edges the distance is half the tile size
	float tileSize_2 = tileSizeSS / 2.0f;
	// from the center to the inside border edges the distance is tileSize/2 - gridWidth
	float centerToInsideBorder = tileSize_2 - gridWidth;
	// from the center to the outside border edges the distance is tileSize/2 + gridWidth
	float centerToOutsideBorder = tileSize_2 + gridWidth;

	//inside
	float distanceToRightInsideBorder = gl_FragCoord.x - (tileCenterSS.x + centerToInsideBorder);
	float distanceToLeftInsideBorder = (tileCenterSS.x - centerToInsideBorder) - gl_FragCoord.x;
	float distanceToToptInsideBorder = gl_FragCoord.y - (tileCenterSS.y + centerToInsideBorder);
	float distanceToBottomInsideBorder = (tileCenterSS.y - centerToInsideBorder) - gl_FragCoord.y;

	vec4 distanceToInsideBorder = vec4(distanceToLeftInsideBorder, distanceToBottomInsideBorder, distanceToRightInsideBorder, distanceToToptInsideBorder);
	vec4 distanceToInsideBorderNorm = distanceToInsideBorder / gridWidth;

	//outside
	vec2 leftBottomCorner = tileCenterSS - tileSize_2;
    vec2 leftTopCorner = vec2(tileCenterSS.x - tileSize_2, tileCenterSS.y + tileSize_2);
    vec2 rightBottomCorner = vec2(tileCenterSS.x + tileSize_2, tileCenterSS.y - tileSize_2);
    vec2 rightTopCorner = tileCenterSS + tileSize_2;

	float distanceToRightOutsideBorder = (tileCenterSS.x + centerToOutsideBorder) - gl_FragCoord.x;
	float distanceToLeftOutsideBorder =  gl_FragCoord.x - (tileCenterSS.x - centerToOutsideBorder);
	float distanceToToptOutsideBorder = (tileCenterSS.y + centerToOutsideBorder) - gl_FragCoord.y;
	float distanceToBottomOutsideBorder = gl_FragCoord.y - (tileCenterSS.y - centerToOutsideBorder);

	//neighbours in vector are neghbours in geometry
	vec4 distanceToOutsideBorder = vec4(distanceToLeftOutsideBorder, distanceToBottomOutsideBorder, distanceToRightOutsideBorder, distanceToToptOutsideBorder);
	vec4 distanceToOutsideBorderNorm = distanceToOutsideBorder / gridWidth;

	//left
	//inside
	if(distanceToInsideBorder.x <= gridWidth && 
		pointLeftOfLine(leftTopCorner, vec2(leftTopCorner.x + gridWidth, leftTopCorner.y - gridWidth), vec2(gl_FragCoord)) &&
		pointLeftOfLine(vec2(leftBottomCorner.x + gridWidth, leftBottomCorner.y + gridWidth), leftBottomCorner, vec2(gl_FragCoord)))
	{
		gridTexture = vec4(borderColor, distanceToInsideBorderNorm.x);
	}		
	//outside if the left neighbour is empty
	if(neighbourValue.x == 0 && 
		distanceToOutsideBorder.x <= gridWidth && 
		pointLeftOfLine(vec2(leftTopCorner.x - gridWidth, leftTopCorner.y-gridWidth), leftTopCorner, vec2(gl_FragCoord)) &&
		pointLeftOfLine(leftBottomCorner, vec2(leftBottomCorner.x - gridWidth, leftBottomCorner.y + gridWidth), vec2(gl_FragCoord)))
	{
		gridTexture = vec4(borderColor, distanceToOutsideBorderNorm.x);
	}

	//bottom
	//inside
	if(distanceToInsideBorder.y <= gridWidth && 
		pointLeftOfLine(vec2(rightBottomCorner.x - gridWidth, rightBottomCorner.y + gridWidth), rightBottomCorner, vec2(gl_FragCoord)) &&
		pointLeftOfLine(leftBottomCorner, vec2(leftBottomCorner.x + gridWidth, leftBottomCorner.y + gridWidth), vec2(gl_FragCoord)))
	{
		gridTexture = vec4(borderColor, distanceToInsideBorderNorm.y);
	}
	//outside if bottom neghbour is empty
	if(neighbourValue.y == 0 &&
		distanceToOutsideBorder.y <= gridWidth && 
		pointLeftOfLine(rightBottomCorner, vec2(rightBottomCorner.x - gridWidth, rightBottomCorner.y - gridWidth),vec2(gl_FragCoord)) &&
		pointLeftOfLine(vec2(leftBottomCorner.x + gridWidth, leftBottomCorner.y - gridWidth), leftBottomCorner, vec2(gl_FragCoord)))
	{
		gridTexture = vec4(borderColor, distanceToOutsideBorderNorm.y);
	}

	//right
	//inside 
	if(distanceToInsideBorder.z <= gridWidth && 
		pointLeftOfLine(rightBottomCorner, vec2(rightBottomCorner.x - gridWidth, rightBottomCorner.y + gridWidth), vec2(gl_FragCoord)) &&
		pointLeftOfLine(vec2(rightTopCorner.x - gridWidth, rightTopCorner.y - gridWidth), rightTopCorner, vec2(gl_FragCoord)))
	{
		gridTexture = vec4(borderColor, distanceToInsideBorderNorm.z);
	}
	//outside if right neighbour is empty
	if(neighbourValue.z == 0 &&
		distanceToOutsideBorder.z <= gridWidth && 
		pointLeftOfLine(vec2(rightBottomCorner.x + gridWidth, rightBottomCorner.y + gridWidth), rightBottomCorner, vec2(gl_FragCoord)) &&
		pointLeftOfLine(rightTopCorner, vec2(rightTopCorner.x + gridWidth, rightTopCorner.y - gridWidth), vec2(gl_FragCoord)))
	{
		gridTexture = vec4(borderColor, distanceToOutsideBorderNorm.z);
	}
	
	//top
	//inside
	if(distanceToInsideBorder.w <= gridWidth && 
		pointLeftOfLine(vec2(leftTopCorner.x + gridWidth, leftTopCorner.y - gridWidth), leftTopCorner, vec2(gl_FragCoord)) &&
		pointLeftOfLine(rightTopCorner, vec2(rightTopCorner.x - gridWidth, rightTopCorner.y - gridWidth), vec2(gl_FragCoord)))
	{
		gridTexture = vec4(borderColor, distanceToInsideBorderNorm.w);
	}
	//outside if top neghbour is empty
	if(neighbourValue.w == 0 &&
		distanceToOutsideBorder.w <= gridWidth && 
		pointLeftOfLine(leftTopCorner, vec2(leftTopCorner.x + gridWidth, leftTopCorner.y + gridWidth), vec2(gl_FragCoord)) &&
		pointLeftOfLine(vec2(rightTopCorner.x - gridWidth, rightTopCorner.y + gridWidth), rightTopCorner, vec2(gl_FragCoord)))
	{
		gridTexture = vec4(borderColor, distanceToOutsideBorderNorm.w);
	}

}