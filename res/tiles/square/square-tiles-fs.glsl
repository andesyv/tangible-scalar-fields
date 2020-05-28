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

uniform float normalsFactor;
uniform float tileHeightMult;
uniform float borderWidth;
uniform bool showBorder;
uniform bool invertPyramid;

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
        tileNormal /= normalsFactor;

        kdeHeight = float(tileNormals[int((squareX*(maxTexCoordY+1) + squareY) * 5 + 4)]);
        kdeHeight /= normalsFactor;
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
        //height at tile center
        float tileCenterZ = kdeHeight * tileHeightMult;
        if(invertPyramid){
            tileCenterZ *= -1;
        }

        vec3 tileCenter3D = vec3(tileCenter2D, tileCenterZ);

        if(showBorder){
            // BORDER-------------------------------------

            // 1) get lowest corner point
            float heightLeftBottomCorner = getHeightOfPointOnSurface(vec2(leftBottomCorner), tileCenter3D, lightingNormal);
            float heightLeftTopCorner = getHeightOfPointOnSurface(vec2(leftTopCorner), tileCenter3D, lightingNormal);
            float heightRightBottomCorner = getHeightOfPointOnSurface(vec2(rightBottomCorner), tileCenter3D, lightingNormal);
            float heightRightTopCorner = getHeightOfPointOnSurface(vec2(rightTopCorner), tileCenter3D, lightingNormal);

            float minHeightCorner = min(min(heightLeftBottomCorner, heightLeftTopCorner), min(heightRightBottomCorner, heightRightTopCorner));
            if(invertPyramid){
                //minHeightCorner becomes maxHeightCorner if pyramid is inverted
                minHeightCorner = max(max(heightLeftBottomCorner, heightLeftTopCorner), max(heightRightBottomCorner, heightRightTopCorner));
            }

            // 2) get z value of border plane center by multiplying tileCenterZ-minHeightCorner with borderWidth and then adding minHeightCorner again
            // get border plane
            float heightOffset = tileCenterZ - minHeightCorner;
            float borderPlaneCenterZ = (tileCenterZ - heightOffset) * borderWidth + heightOffset;
            
            vec3 borderPlaneCenter = vec3(tileCenter2D, borderPlaneCenterZ);

            // 3) get intersections of plane with pyramid
            // pyramid top = tileCenter3D
            // lineDir = normalize(pyramidTop - Corner)

            vec3 leftBottomInside = linePlaneIntersection(lightingNormal, borderPlaneCenter, normalize(tileCenter3D - leftBottomCorner), tileCenter3D);        
            vec3 leftTopInside = linePlaneIntersection(lightingNormal, borderPlaneCenter, normalize(tileCenter3D - leftTopCorner), tileCenter3D);        
            vec3 rightBottomInside = linePlaneIntersection(lightingNormal, borderPlaneCenter, normalize(tileCenter3D - rightBottomCorner), tileCenter3D);        
            vec3 rightTopInside = linePlaneIntersection(lightingNormal, borderPlaneCenter, normalize(tileCenter3D - rightTopCorner), tileCenter3D);        

            //--------------------------------------------
            bool debugNormals = false;
            if(debugNormals){
                squareTilesTexture = vec4(lightingNormal, 1.0f);
            }
            else{
                if(pointInBorder(fragmentPos, leftBottomInside, leftTopInside, rightBottomInside, rightTopInside)){
                    //check which side
                    //left
                    if(pointLeftOfLine(vec2(leftTopInside), vec2(leftBottomInside), vec2(fragmentPos)) 
                    && pointLeftOfLine(vec2(leftBottomInside),vec2(leftBottomCorner),vec2(fragmentPos))
                    && pointLeftOfLine(vec2(leftTopCorner),vec2(leftTopInside),vec2(fragmentPos))){
                        
                        //compute surface normal using 2 corner points of inside and 1 corner point of outside
                        lightingNormal = calcPlaneNormal(leftBottomInside, leftTopInside, leftBottomCorner);
                        
                        // fragment height
                        fragmentPos.z = getHeightOfPointOnSurface(vec2(fragmentPos), leftBottomCorner, lightingNormal);   
                    }
                    //right
                    else if(pointLeftOfLine(vec2(rightBottomInside),vec2(rightTopInside), vec2(fragmentPos))
                    && pointLeftOfLine(vec2(rightBottomCorner),vec2(rightBottomInside),vec2(fragmentPos))
                    && pointLeftOfLine(vec2(rightTopInside),vec2(rightTopCorner),vec2(fragmentPos))){

                        //compute surface normal using 2 corner points of inside and 1 corner point of outside
                        lightingNormal = calcPlaneNormal(rightBottomInside, rightBottomCorner, rightTopInside);

                        // fragment height
                        fragmentPos.z = getHeightOfPointOnSurface(vec2(fragmentPos), rightBottomCorner, lightingNormal);
                    }
                    //bottom
                    else if(pointLeftOfLine(vec2(leftBottomInside), vec2(rightBottomInside), vec2(fragmentPos))){
                        //compute surface normal using 2 corner points of inside and 1 corner point of outside
                        lightingNormal = calcPlaneNormal(leftBottomInside, leftBottomCorner, rightBottomInside);

                        // fragment height
                        fragmentPos.z = getHeightOfPointOnSurface(vec2(fragmentPos), leftBottomCorner, lightingNormal);
                    }
                    //top
                    else if(pointLeftOfLine(vec2(rightTopInside), vec2(leftTopInside), vec2(fragmentPos))){
                        
                        //compute surface normal using 2 corner points of inside and 1 corner point of outside
                        lightingNormal = calcPlaneNormal(leftTopInside, rightTopInside, leftTopCorner);

                        // fragment height
                        fragmentPos.z = getHeightOfPointOnSurface(vec2(fragmentPos), leftTopCorner, lightingNormal);
                    }
                    //discard;
                }
                //point is on the inside
                else{
                    // fragemnt height
                    fragmentPos.z = getHeightOfPointOnSurface(vec2(fragmentPos), tileCenter3D, lightingNormal);         
                }
            }
        }
        else{

            // fragment height
            fragmentPos.z = getHeightOfPointOnSurface(vec2(fragmentPos), tileCenter3D, lightingNormal);
            //debug
            //distance to center
            float distCenter = length(vec2(fragmentPos) - tileCenter2D); 
            float normDistCenter = mapInterval_O(distCenter, 0, int(ceil(tileSizeScreenSpace/2.0f)), 0.0f, 1.0f);

            /*
            //when there are a lot of tiles (small tileSize) this affects fps massively negativ
            float maxKdeHeight = 0.0f;
            for(int i = 0; i < maxTexCoordX+1; i++){
                for(int j = 0; j < maxTexCoordY+1; j++){
                    float tmpKdeHeight = float(tileNormals[int((i*(maxTexCoordY+1) + j) * 5 + 4)]);
                    tmpKdeHeight /= normalsFactor;
                    maxKdeHeight = max(tmpKdeHeight, maxKdeHeight);
                }
            }

            float normZ = mapInterval_O(fragmentPos.z, 0, int(maxKdeHeight), 0.0f, 1.0f);

            squareTilesTexture = vec4(normZ, 0.0f, 0.0f, 1.0f);  */
        }
    #endif
    // PHONG LIGHTING ----------------------------------------------------------------------------------------------

    squareTilesTexture.rgb = calculatePhongLighting(lightColor, lightPos, fragmentPos, lightingNormal, viewPos) * squareTilesTexture.rgb;

}
