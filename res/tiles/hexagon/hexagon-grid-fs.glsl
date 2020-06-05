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

	/*
		-----
	  /	  |___\
	  \	   __ /
		-----

		upper horizontal line inside hexagon = tileSize  	(= distance to center corner points)
		lower horizontal line inside hexagon = tileSize / 2 (= x component of distance to top and bottom corner points)
		vertical line inside hexagon = vertical space / 2   (= y component of distance to top and bottom corner points)
	*/
	float tileSize_2 = tileSizeSS / 2.0f;
	float vertical_space_2 = sqrt(3) * tileSize_2;

	// from the center to the inside border edges the distance is tileSize/2 - gridWidth
	float tileSizeInside = tileSizeSS - gridWidth;
	float tileSizeInside_2 = tileSizeInside / 2.0f;
	float vertical_space_inside_2 = sqrt(3) * tileSizeInside_2;

	// from the center to the outside border edges the distance is tileSize/2 + gridWidth
	float tileSizeOutside = tileSizeSS + gridWidth;
	float tileSizeOutside_2 = tileSizeOutside / 2.0f;
	float vertical_space_outside_2 = sqrt(3) * tileSizeOutside;

	//hexagon tile corner points
 	vec2 leftBottomCorner = vec2(tileCenterSS + vec2(-tileSize_2, -vertical_space_2));     
	vec2 leftCenterCorner = vec2(tileCenterSS + vec2(-tileSizeSS, 0.0f));
	vec2 leftTopCorner = vec2(tileCenterSS + vec2(-tileSize_2, vertical_space_2));     
	vec2 rightBottomCorner = vec2(tileCenterSS + vec2(tileSize_2, -vertical_space_2));     
	vec2 rightCenterCorner = vec2(tileCenterSS + vec2(tileSizeSS, 0.0f));
	vec2 rightTopCorner = vec2(tileCenterSS + vec2(tileSize_2, vertical_space_2));

	vec2 leftBottomCornerInside = vec2(tileCenterSS + vec2(-tileSizeInside_2, -vertical_space_inside_2));     
	vec2 leftCenterCornerInside = vec2(tileCenterSS + vec2(-tileSizeInside, 0.0f));
	vec2 leftTopCornerInside = vec2(tileCenterSS + vec2(-tileSizeInside_2, vertical_space_inside_2));     
	vec2 rightBottomCornerInside = vec2(tileCenterSS + vec2(tileSizeInside_2, -vertical_space_inside_2));     
	vec2 rightCenterCornerInside = vec2(tileCenterSS + vec2(tileSizeInside, 0.0f));
	vec2 rightTopCornerInside = vec2(tileCenterSS + vec2(tileSizeInside_2, vertical_space_inside_2));

	//inside
	float distanceToLeftTopInsideBorder = distancePointToLine(vec3(gl_FragCoord), vec3(leftCenterCornerInside, 0), vec3(leftTopCornerInside, 0));
	float distanceToLeftBottomInsideBorder = distancePointToLine(vec3(gl_FragCoord), vec3(leftBottomCornerInside, 0), vec3(leftCenterCornerInside, 0));
	float distanceToBottomInsideBorder = (tileCenterSS.y - vertical_space_inside_2) - gl_FragCoord.y;
	float distanceToRightBottomInsideBorder = distancePointToLine(vec3(gl_FragCoord), vec3(rightCenterCornerInside, 0), vec3(rightBottomCornerInside, 0));
	float distanceToRightTopInsideBorder = distancePointToLine(vec3(gl_FragCoord), vec3(rightTopCornerInside, 0), vec3(rightCenterCornerInside, 0));
	float distanceToTopInsideBorder = gl_FragCoord.y - (tileCenterSS.y + vertical_space_inside_2);
	
	//left top, left bottom, bottom, right bottom, right top, top
	float distancesInside[6];
	float distancesInsideNorm[6];

	distancesInside[0] = distanceToLeftTopInsideBorder;
	distancesInside[1] = distanceToLeftBottomInsideBorder;
	distancesInside[2] = distanceToBottomInsideBorder;
	distancesInside[3] = distanceToRightBottomInsideBorder;
	distancesInside[4] = distanceToRightTopInsideBorder;
	distancesInside[5] = distanceToTopInsideBorder;

	for(int i = 0; i < 6; i++){
		distancesInsideNorm[i] = distancesInside[i] / gridWidth;
	}

	//left top
	//inside
	if(distancesInside[0] <= gridWidth && 
		pointLeftOfLine(leftTopCornerInside, leftCenterCornerInside, vec2(gl_FragCoord)) && 
		pointLeftOfLine(leftTopCorner, leftTopCornerInside, vec2(gl_FragCoord)) &&
		pointLeftOfLine(leftCenterCornerInside, leftCenterCorner, vec2(gl_FragCoord)))
	{
		gridTexture = vec4(borderColor, distancesInsideNorm[0]);
	}	

	//left bottom
	//inside
	if(distancesInside[1] <= gridWidth && 
		pointLeftOfLine(leftCenterCornerInside, leftBottomCornerInside, vec2(gl_FragCoord)) && 
		pointLeftOfLine(leftCenterCorner, leftCenterCornerInside, vec2(gl_FragCoord)) &&
		pointLeftOfLine(leftBottomCornerInside, leftBottomCorner, vec2(gl_FragCoord)))
	{
		gridTexture = vec4(borderColor, distancesInsideNorm[1]);
	}	

	//bottom
	//inside
	if(distancesInside[2] <= gridWidth && 
		pointLeftOfLine(leftBottomCorner, leftBottomCornerInside, vec2(gl_FragCoord)) &&
		pointLeftOfLine(rightBottomCornerInside, rightBottomCorner, vec2(gl_FragCoord)))
	{
		gridTexture = vec4(borderColor, distancesInsideNorm[2]);
	}

	//right bottom
	//inside
	if(distancesInside[3] <= gridWidth && 
		pointLeftOfLine(rightBottomCornerInside, rightCenterCornerInside, vec2(gl_FragCoord)) && 
		pointLeftOfLine(rightCenterCornerInside, rightCenterCorner, vec2(gl_FragCoord)) &&
		pointLeftOfLine(rightBottomCorner, rightBottomCornerInside, vec2(gl_FragCoord)))
	{
		gridTexture = vec4(borderColor, distancesInsideNorm[3]);
	}
	
	//right top
	//inside
	if(distancesInside[4] <= gridWidth && 
		pointLeftOfLine(rightCenterCornerInside, rightTopCornerInside, vec2(gl_FragCoord)) && 
		pointLeftOfLine(rightTopCornerInside, rightTopCorner, vec2(gl_FragCoord)) &&
		pointLeftOfLine(rightCenterCorner, rightCenterCornerInside, vec2(gl_FragCoord)))
	{
		gridTexture = vec4(borderColor, distancesInsideNorm[4]);
	}

	//top
	//inside
	if(distancesInside[5] <= gridWidth && 
		pointLeftOfLine(leftTopCornerInside, leftTopCorner, vec2(gl_FragCoord)) &&
		pointLeftOfLine(rightTopCorner, rightTopCornerInside, vec2(gl_FragCoord)))
	{
		gridTexture = vec4(borderColor, distancesInsideNorm[5]);
	}
}