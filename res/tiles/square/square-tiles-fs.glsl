#version 450
#include "/defines.glsl"
#include "/globals.glsl"

//--in
layout(pixel_center_integer) in vec4 gl_FragCoord;

layout(std430, binding = 0) buffer tileNormalsBuffer
{
    int tileNormals[];
};

layout(std430, binding = 1) buffer valueMaxBuffer
{
    uint maxAccumulate;
    uint maxPointAlpha;
};

struct box{
    float left;
    float right;
    float top;
    float bottom;
} insideBoundingBox;

in vec4 boundsScreenSpace;
in float tileSizeScreenSpace;

uniform sampler2D accumulateTexture;
uniform sampler2D tilesDiscrepancyTexture;

// 1D color map parameters
uniform sampler1D colorMapTexture;
uniform int textureWidth;

//min = 0
uniform int maxTexCoordX;
uniform int maxTexCoordY;

uniform float bufferAccumulationFactor;
uniform float tileHeightMult;
uniform float borderWidth;
uniform bool showBorder;
uniform bool invertPyramid;
uniform float blendRange;

//Lighting----------------------
uniform vec3 lightPos; 
uniform vec3 viewPos; //is vec3(0,0,0) because we work in viewspace
uniform vec3 lightColor;
//----------------------

//--out
layout (location = 0) out vec4 squareTilesTexture;

bool pointInBorder(vec3 p, vec3 leftBottomInside, vec3 leftTopInside, vec3 rightBottomInside, vec3 rightTopInside){
    
    return pointLeftOfLine(vec2(leftTopInside), vec2(leftBottomInside), vec2(p))
            || pointLeftOfLine(vec2(rightBottomInside),vec2(rightTopInside),vec2(p))
            || pointLeftOfLine(vec2(leftBottomInside), vec2(rightBottomInside), vec2(p))
            || pointLeftOfLine(vec2(rightTopInside), vec2(leftTopInside), vec2(p));
}

void main()
{

    // to get intervals from 0 to maxTexCoord, we map the original Point interval to maxTexCoord+1
    // If the current index = maxIndex+1, we take the maxTexCoord instead
    int squareX = min(maxTexCoordX, mapInterval(gl_FragCoord.x, boundsScreenSpace[2], boundsScreenSpace[0], maxTexCoordX+1));
    int squareY = min(maxTexCoordY, mapInterval(gl_FragCoord.y, boundsScreenSpace[3], boundsScreenSpace[1], maxTexCoordY+1));

    // get value from accumulate texture
    float squareValue = texelFetch(accumulateTexture, ivec2(squareX,squareY), 0).r;

    // we don't want to render empty squares
    if(squareValue < 0.00000001){
        discard;
    }

	// read from SSBO and convert back to float: --------------------------------------------------------------------
	// - https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/intBitsToFloat.xhtml
    // +1 because else we cannot map the maximum value itself
    float floatMaxAccumulate = uintBitsToFloat(maxAccumulate) + 1;

    //color squares according to index
    squareTilesTexture = vec4(float(squareX/float(maxTexCoordX)),float(squareY/float(maxTexCoordY)),0.0f,1.0f);

    // color square according to value
	#ifdef COLORMAP
		int colorTexelCoord = mapInterval(squareValue, 0, floatMaxAccumulate, textureWidth);
		squareTilesTexture.rgb = texelFetch(colorMapTexture, colorTexelCoord, 0).rgb;
	#endif

    #ifdef RENDER_DISCREPANCY
        //discrepancy is set as opacity
        float tilesDiscrepancy = texelFetch(tilesDiscrepancyTexture, ivec2(squareX,squareY), 0).r;
        squareTilesTexture *= tilesDiscrepancy;
    #endif

    // REGRESSION PLANE------------------------------------------------------------------------------------------------
    vec4 tileNormal = vec4(0.0f,0.0f,0.0f,1.0f);
    float kdeHeight = 0.0f;
    vec3 lightingNormal = vec3(0.0f,0.0f,0.0f);
    vec3 fragmentPos = vec3(gl_FragCoord);
    #ifdef RENDER_TILE_NORMALS
        for(int i = 0; i < 4; i++){
            tileNormal[i] = float(tileNormals[int((squareX*(maxTexCoordY+1) + squareY) * 5 + i)]);
        }
        tileNormal /= bufferAccumulationFactor;

        kdeHeight = float(tileNormals[int((squareX*(maxTexCoordY+1) + squareY) * 5 + 4)]);
        kdeHeight /= bufferAccumulationFactor;
        kdeHeight /= tileNormal.w;
       
        // LIGHTING NORMAL ------------------------
        lightingNormal = normalize(vec3(tileNormal.x, tileNormal.y, tileNormal.w));
        //-----------------------------------------

        //Corner Of Square (z=0)
        vec3 leftBottomCorner = vec3(squareX * tileSizeScreenSpace + boundsScreenSpace[2], squareY * tileSizeScreenSpace + boundsScreenSpace[3], 0.0f);
        vec3 leftTopCorner = vec3(leftBottomCorner.x, leftBottomCorner.y+tileSizeScreenSpace, 0.0f);
        vec3 rightBottomCorner = vec3(leftBottomCorner.x + tileSizeScreenSpace, leftBottomCorner.y, 0.0f);
        vec3 rightTopCorner = vec3(rightBottomCorner.x, rightBottomCorner.y + tileSizeScreenSpace, 0.0f);

        // move size/2 up and right
        vec2 tileCenter2D = vec2(leftBottomCorner) + tileSizeScreenSpace / 2.0f;
        //height of pyramid
        float pyramidHeight = kdeHeight * tileHeightMult;
        if(invertPyramid){
            pyramidHeight *= -1;
        }

        vec3 tileCenter3D;
        vec3 pyramidTop = vec3(tileCenter2D, pyramidHeight);

        if(showBorder){
            // BORDER-------------------------------------

            // 1) get lowest corner point of regression plane when sitting on top of the pyramid
            float heightLeftBottomCorner = getHeightOfPointOnSurface(vec2(leftBottomCorner), pyramidTop, lightingNormal);
            float heightLeftTopCorner = getHeightOfPointOnSurface(vec2(leftTopCorner), pyramidTop, lightingNormal);
            float heightRightBottomCorner = getHeightOfPointOnSurface(vec2(rightBottomCorner), pyramidTop, lightingNormal);
            float heightRightTopCorner = getHeightOfPointOnSurface(vec2(rightTopCorner), pyramidTop, lightingNormal);

            float heightOffset;
            //we need a small offset, because we cannot set an inside corner point to height 0
            //this would lead to the effect that the regresseion plane is overlapping with one of the border planes
            //this on the other hand pused all inside corner points to the center of the tile, because there is no more cutting plane
            //between regression plane and pyramid.
            //if thats the case, we can not longer compute if a point is in the border, because there are no more line between the inside corner points
            float offset = 0.01f;
            if(invertPyramid){
                float maxHeightCorner = max(max(heightLeftBottomCorner, heightLeftTopCorner), max(heightRightBottomCorner, heightRightTopCorner));
               //we need to clamp the pyramidHeight to pyramidHeight + maxHeightCorner*-1 to avoid that the maximum Height Corner is positive
                if(maxHeightCorner > -offset){
                    float maxCornerNegative = maxHeightCorner > 0 ? maxHeightCorner * (-1) - offset : maxHeightCorner - offset;
                    pyramidHeight += maxCornerNegative;
                    pyramidTop = vec3(tileCenter2D, pyramidHeight);
                    maxHeightCorner = -offset;
                }
                heightOffset = pyramidHeight - maxHeightCorner;
            }
            else{
                float minHeightCorner = min(min(heightLeftBottomCorner, heightLeftTopCorner), min(heightRightBottomCorner, heightRightTopCorner));
                //we need to clamp the pyramidHeight to pyramidHeight + abs(minHeightCorner) to avoid that the minimum Height Corner is negative
                if(minHeightCorner < offset){
                    float minCornerAbs = abs(minHeightCorner) + offset;
                    pyramidHeight += minCornerAbs;
                    pyramidTop = vec3(tileCenter2D, pyramidHeight);
                    minHeightCorner = offset;
                }
                heightOffset = pyramidHeight - minHeightCorner;
            }

            // 2) get z value of regression plane center by multiplying pyramidHeight-heightOffset with borderWidth and then adding heightOffset again
            // get regression plane
            float tileCenterZ = (pyramidHeight - heightOffset) * borderWidth + heightOffset;
            
            tileCenter3D = vec3(tileCenter2D, tileCenterZ);

            // 3) get intersections of plane with pyramid
            // lineDir = normalize(pyramidTop - Corner)
            vec3 leftBottomInside = linePlaneIntersection(lightingNormal, tileCenter3D, normalize(pyramidTop - leftBottomCorner), pyramidTop);        
            vec3 leftTopInside = linePlaneIntersection(lightingNormal, tileCenter3D, normalize(pyramidTop - leftTopCorner), pyramidTop);        
            vec3 rightBottomInside = linePlaneIntersection(lightingNormal, tileCenter3D, normalize(pyramidTop - rightBottomCorner), pyramidTop);        
            vec3 rightTopInside = linePlaneIntersection(lightingNormal, tileCenter3D, normalize(pyramidTop - rightTopCorner), pyramidTop);        

            //--------------------------------------------
            if(pointInBorder(fragmentPos, leftBottomInside, leftTopInside, rightBottomInside, rightTopInside)){
                //check which side
                //left
                if(pointLeftOfLine(vec2(leftTopInside), vec2(leftBottomInside), vec2(fragmentPos)) 
                && pointLeftOfLine(vec2(leftBottomInside),vec2(leftBottomCorner),vec2(fragmentPos))
                && pointLeftOfLine(vec2(leftTopCorner),vec2(leftTopInside),vec2(fragmentPos))){

                    squareTilesTexture.rgb = calculateBorderColor(fragmentPos, lightingNormal, leftBottomCorner, leftBottomInside, leftTopInside, 
                                            tileCenter3D, blendRange, squareTilesTexture, lightColor, lightPos, viewPos);  
                }
                //right
                else if(pointLeftOfLine(vec2(rightBottomInside),vec2(rightTopInside), vec2(fragmentPos))
                && pointLeftOfLine(vec2(rightBottomCorner),vec2(rightBottomInside),vec2(fragmentPos))
                && pointLeftOfLine(vec2(rightTopInside),vec2(rightTopCorner),vec2(fragmentPos))){

                    squareTilesTexture.rgb = calculateBorderColor(fragmentPos, lightingNormal, rightTopCorner, rightTopInside, rightBottomInside,
                                            tileCenter3D, blendRange, squareTilesTexture, lightColor, lightPos, viewPos);
                }
                //bottom
                else if(pointLeftOfLine(vec2(leftBottomInside), vec2(rightBottomInside), vec2(fragmentPos))){
                    
                    squareTilesTexture.rgb = calculateBorderColor(fragmentPos, lightingNormal, rightBottomCorner, rightBottomInside, leftBottomInside, 
                                            tileCenter3D, blendRange, squareTilesTexture, lightColor, lightPos, viewPos);
                }
                //top
                else if(pointLeftOfLine(vec2(rightTopInside), vec2(leftTopInside), vec2(fragmentPos))){

                    squareTilesTexture.rgb = calculateBorderColor(fragmentPos, lightingNormal, leftTopCorner, leftTopInside, rightTopInside, 
                                            tileCenter3D, blendRange, squareTilesTexture, lightColor, lightPos, viewPos);
                }
            }
            //point is on the inside
            else{
                // fragemnt height
                fragmentPos.z = getHeightOfPointOnSurface(vec2(fragmentPos), tileCenter3D, lightingNormal); 

                // PHONG LIGHTING 
                squareTilesTexture.rgb = calculatePhongLighting(lightColor, lightPos, fragmentPos, lightingNormal, viewPos) * squareTilesTexture.rgb;
            }
        }
        else{

            //regression plane center is on height of pyramid
            tileCenter3D = pyramidTop;

            // fragment height
            fragmentPos.z = getHeightOfPointOnSurface(vec2(fragmentPos), tileCenter3D, lightingNormal);
            
            // PHONG LIGHTING 
            squareTilesTexture.rgb = calculatePhongLighting(lightColor, lightPos, fragmentPos, lightingNormal, viewPos) * squareTilesTexture.rgb;
        }
    #endif

}
