#version 450
#include "/defines.glsl"
#include "/globals.glsl"

layout (location = 0) out vec4 gridTexture;

uniform vec3 gridColor;
uniform float gridWidth;

//in screen space
in float tileSizeSS;
in vec2 tileCenterSS;

// draws the grid, if gl_FragCoords is inside the gridWidth
// point naming:  
//	top 	2 <--- 1   
//	bottom 	1 ---> 2
//	counter clockwise
void drawInsideGrid(float distanceInside, float distanceInsideNorm, vec2 corner1, vec2 corner2, vec2 cornerInside1, vec2 cornerInside2){
	if(distanceInside <= gridWidth && 
		pointLeftOfLine(cornerInside1, cornerInside2, vec2(gl_FragCoord)) && 
		pointLeftOfLine(corner1, cornerInside1, vec2(gl_FragCoord)) &&
		pointLeftOfLine(cornerInside2, corner2, vec2(gl_FragCoord)))
	{
		gridTexture = vec4(gridColor, distanceInsideNorm);
	}	
}

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

	//inside grid corner points
	vec2 leftBottomCornerInside = vec2(tileCenterSS + vec2(-tileSizeInside_2, -vertical_space_inside_2));     
	vec2 leftCenterCornerInside = vec2(tileCenterSS + vec2(-tileSizeInside, 0.0f));
	vec2 leftTopCornerInside = vec2(tileCenterSS + vec2(-tileSizeInside_2, vertical_space_inside_2));     
	vec2 rightBottomCornerInside = vec2(tileCenterSS + vec2(tileSizeInside_2, -vertical_space_inside_2));     
	vec2 rightCenterCornerInside = vec2(tileCenterSS + vec2(tileSizeInside, 0.0f));
	vec2 rightTopCornerInside = vec2(tileCenterSS + vec2(tileSizeInside_2, vertical_space_inside_2));

	//left top, left bottom, bottom, right bottom, right top, top
	float distancesInside[6];
	float distancesInsideNorm[6];

	distancesInside[0] = distancePointToLine(vec3(gl_FragCoord), vec3(leftCenterCornerInside, 0), vec3(leftTopCornerInside, 0));
	distancesInside[1] = distancePointToLine(vec3(gl_FragCoord), vec3(leftBottomCornerInside, 0), vec3(leftCenterCornerInside, 0));
	distancesInside[2] = (tileCenterSS.y - vertical_space_inside_2) - gl_FragCoord.y;
	distancesInside[3] = distancePointToLine(vec3(gl_FragCoord), vec3(rightCenterCornerInside, 0), vec3(rightBottomCornerInside, 0));
	distancesInside[4] = distancePointToLine(vec3(gl_FragCoord), vec3(rightTopCornerInside, 0), vec3(rightCenterCornerInside, 0));
	distancesInside[5] = gl_FragCoord.y - (tileCenterSS.y + vertical_space_inside_2);
	
	for(int i = 0; i < 6; i++){
		distancesInsideNorm[i] = distancesInside[i] / gridWidth;
	}

	//left top
	//inside
	drawInsideGrid(distancesInside[0], distancesInsideNorm[0], leftTopCorner, leftCenterCorner, leftTopCornerInside, leftCenterCornerInside);

	//left bottom
	//inside
	drawInsideGrid(distancesInside[1], distancesInsideNorm[1], leftCenterCorner, leftBottomCorner, leftCenterCornerInside, leftBottomCornerInside);
	
	//bottom
	//inside
	drawInsideGrid(distancesInside[2], distancesInsideNorm[2], leftBottomCorner, rightBottomCorner, leftBottomCornerInside, rightBottomCornerInside);

	//right bottom
	//inside
	drawInsideGrid(distancesInside[3], distancesInsideNorm[3], rightBottomCorner, rightCenterCorner, rightBottomCornerInside, rightCenterCornerInside);
	
	//right top
	//inside
	drawInsideGrid(distancesInside[4], distancesInsideNorm[4], rightCenterCorner, rightTopCorner, rightCenterCornerInside, rightTopCornerInside);
	
	//top
	//inside
	drawInsideGrid(distancesInside[5], distancesInsideNorm[5], rightTopCorner, leftTopCorner, rightTopCornerInside, leftTopCornerInside);
}