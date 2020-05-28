#version 450
#include "/defines.glsl"
#include "/globals.glsl"
#include "/hex/globals.glsl"

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

in vec4 boundsScreenSpace;
in vec2 rectSizeScreenSpace;
in float tileSizeScreenSpace;

uniform sampler2D accumulateTexture;
uniform sampler2D tilesDiscrepancyTexture;

// 1D color map parameters
uniform sampler1D colorMapTexture;
uniform int textureWidth;

//min = 0
uniform int maxTexCoordX;
uniform int maxTexCoordY;

uniform int max_rect_col;
uniform int max_rect_row;

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
layout(location = 0) out vec4 hexTilesTexture;

bool pointInBorder(vec3 p, vec3 leftBottomInside, vec3 leftCenterInside, vec3 leftTopInside, vec3 rightBottomInside, vec3 rightCenterInside, vec3 rightTopInside){
    
    return pointLeftOfLine(vec2(leftTopInside), vec2(leftCenterInside), vec2(p))
            || pointLeftOfLine(vec2(leftCenterInside), vec2(leftBottomInside), vec2(p))
            || pointLeftOfLine(vec2(leftBottomInside), vec2(rightBottomInside), vec2(p))
            || pointLeftOfLine(vec2(rightBottomInside), vec2(rightCenterInside),vec2(p))
            || pointLeftOfLine(vec2(rightCenterInside), vec2(rightTopInside), vec2(p))
            || pointLeftOfLine(vec2(rightTopInside), vec2(leftTopInside), vec2(p));
}

void main()
{
    vec2 minBounds = vec2(boundsScreenSpace[2], boundsScreenSpace[3]);
    vec2 maxBounds = vec2(boundsScreenSpace[0], boundsScreenSpace[1]);

    if(discardOutsideOfGridFragments(vec2(gl_FragCoord), minBounds, maxBounds, rectSizeScreenSpace, max_rect_col, max_rect_row)){
        discard;
    }

    vec2 hex = matchPointWithHexagon(vec2(gl_FragCoord), max_rect_col, max_rect_row, rectSizeScreenSpace.x, rectSizeScreenSpace.y,
                                     minBounds, maxBounds);

    // get value from accumulate texture
    float hexValue = texelFetch(accumulateTexture, ivec2(hex.x, hex.y), 0).r;

    // we don't want to render empty hexs
    if (hexValue < 0.00000001)
    {
        discard;
    }

    // read from SSBO and convert back to float: --------------------------------------------------------------------
    // - https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/intBitsToFloat.xhtml
    // +1 because else we cannot map the maximum value itself
    float floatMaxAccumulate = uintBitsToFloat(maxAccumulate) + 1;

    //color hexs according to index
    hexTilesTexture = vec4(float(hex.x / float(maxTexCoordX)), float(hex.y / float(maxTexCoordY)), 0.0f, 1.0f);

// color hex according to value
#ifdef COLORMAP
    int colorTexelCoord = mapInterval(hexValue, 0, floatMaxAccumulate, textureWidth);
    hexTilesTexture.rgb = texelFetch(colorMapTexture, colorTexelCoord, 0).rgb;
#endif

#ifdef RENDER_DISCREPANCY
    //discrepancy is set as opacity
    float tilesDiscrepancy = texelFetch(tilesDiscrepancyTexture, ivec2(hex.x, hex.y), 0).r;
    hexTilesTexture *= tilesDiscrepancy;
#endif

// REGRESSION PLANE------------------------------------------------------------------------------------------------
    vec4 tileNormal = vec4(0.0f,0.0f,0.0f,1.0f);
    vec3 lightingNormal = vec3(0.0f,0.0f,0.0f);
    vec3 fragmentPos = vec3(gl_FragCoord);
    float kdeHeight = 0.0f;
    #ifdef RENDER_TILE_NORMALS
        for(int i = 0; i < 4; i++){
            tileNormal[i] = float(tileNormals[int((hex.x*(maxTexCoordY+1) + hex.y) * 5 + i)]);
        }
        tileNormal /= normalsFactor;

        kdeHeight = float(tileNormals[int((hex.x*(maxTexCoordY+1) + hex.y) * 5 + 4)]);
        kdeHeight /= normalsFactor;
        kdeHeight /= tileNormal.w;

        // LIGHTING NORMAL ------------------------
        //lightingNormal = normalize(vec3(tileNormal.x/tileNormal.w, tileNormal.y/tileNormal.w,0.05f));
        lightingNormal = normalize(vec3(tileNormal.x, tileNormal.y, tileNormal.w));
        //-----------------------------------------

        float horizontal_space = tileSizeScreenSpace * 1.5f;
	    float vertical_space = sqrt(3)*tileSizeScreenSpace;

        float vertical_offset = mod(hex.x, 2) == 0 ? vertical_space : vertical_space/2.0f;

        vec2 tileCenter2D = vec2(hex.x * horizontal_space + boundsScreenSpace[2] + tileSizeScreenSpace, hex.y * vertical_space + boundsScreenSpace[3] + vertical_offset);
        //height at tile center
        float tileCenterZ = kdeHeight * tileHeightMult;
        if(invertPyramid){
            tileCenterZ *= -1;
        }

        vec3 tileCenter3D = vec3(tileCenter2D, tileCenterZ);

        if(showBorder){
            // BORDER-------------------------------------
            //Corner Of Hexagon (z=0)
            vec3 leftBottomCorner = vec3(tileCenter2D + vec2(-tileSizeScreenSpace/2.0f, -vertical_space/2.0f), 0.0f);     
            vec3 leftCenterCorner = vec3(tileCenter2D + vec2(-tileSizeScreenSpace, 0.0f), 0.0f);
            vec3 leftTopCorner = vec3(tileCenter2D + vec2(-tileSizeScreenSpace/2.0f, vertical_space/2.0f), 0.0f);     
            vec3 rightBottomCorner = vec3(tileCenter2D + vec2(tileSizeScreenSpace/2.0f, -vertical_space/2.0f), 0.0f);     
            vec3 rightCenterCorner = vec3(tileCenter2D + vec2(tileSizeScreenSpace, 0.0f), 0.0f);
            vec3 rightTopCorner = vec3(tileCenter2D + vec2(tileSizeScreenSpace/2.0f, vertical_space/2.0f), 0.0f);  
            
            // 1) get lowest corner point
            float heightLeftBottomCorner = getHeightOfPointOnSurface(vec2(leftBottomCorner), tileCenter3D, lightingNormal);
            float heightLeftCenterCorner = getHeightOfPointOnSurface(vec2(leftCenterCorner), tileCenter3D, lightingNormal);
            float heightLeftTopCorner = getHeightOfPointOnSurface(vec2(leftTopCorner), tileCenter3D, lightingNormal);
            float heightRightBottomCorner = getHeightOfPointOnSurface(vec2(rightBottomCorner), tileCenter3D, lightingNormal);
            float heightRightCenterCorner = getHeightOfPointOnSurface(vec2(rightCenterCorner), tileCenter3D, lightingNormal);
            float heightRightTopCorner = getHeightOfPointOnSurface(vec2(rightTopCorner), tileCenter3D, lightingNormal);

            float minLeftCorner = min(min(heightLeftBottomCorner, heightLeftCenterCorner), heightLeftTopCorner);
            float minRightCorner = min(min(heightRightBottomCorner, heightRightCenterCorner), heightRightTopCorner);
            float minHeightCorner = min(minLeftCorner, minRightCorner);
            if(invertPyramid){
                //minHeightCorner becomes maxHeightCorner if pyramid is inverted
                minLeftCorner = max(max(heightLeftBottomCorner, heightLeftCenterCorner), heightLeftTopCorner);
                minRightCorner = max(max(heightRightBottomCorner, heightRightCenterCorner), heightRightTopCorner);
                minHeightCorner = max(minLeftCorner, minRightCorner);
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
            vec3 leftCenterInside = linePlaneIntersection(lightingNormal, borderPlaneCenter, normalize(tileCenter3D - leftCenterCorner), tileCenter3D);        
            vec3 leftTopInside = linePlaneIntersection(lightingNormal, borderPlaneCenter, normalize(tileCenter3D - leftTopCorner), tileCenter3D);        
            vec3 rightBottomInside = linePlaneIntersection(lightingNormal, borderPlaneCenter, normalize(tileCenter3D - rightBottomCorner), tileCenter3D);        
            vec3 rightCenterInside = linePlaneIntersection(lightingNormal, borderPlaneCenter, normalize(tileCenter3D - rightCenterCorner), tileCenter3D);        
            vec3 rightTopInside = linePlaneIntersection(lightingNormal, borderPlaneCenter, normalize(tileCenter3D - rightTopCorner), tileCenter3D);        

            //--------------------------------------------

            if(pointInBorder(fragmentPos, leftBottomInside, leftCenterInside, leftTopInside, rightBottomInside, rightCenterInside, rightTopInside)){
                //check which side
                //left top
                if(pointLeftOfLine(vec2(leftTopInside), vec2(leftCenterInside), vec2(fragmentPos)) 
                && pointLeftOfLine(vec2(leftCenterInside),vec2(leftCenterCorner),vec2(fragmentPos))
                && pointLeftOfLine(vec2(leftTopCorner),vec2(leftTopInside),vec2(fragmentPos))){
                    
                    //compute surface normal using 2 corner points of inside and 1 corner point of outside
                    lightingNormal = calcPlaneNormal(leftCenterInside, leftTopInside, leftCenterCorner);
                    
                    // fragment height
                    fragmentPos.z = getHeightOfPointOnSurface(vec2(fragmentPos), leftCenterCorner, lightingNormal);   
                }
                //left bottom
                else if(pointLeftOfLine(vec2(leftCenterInside), vec2(leftBottomInside), vec2(fragmentPos)) 
                && pointLeftOfLine(vec2(leftBottomInside),vec2(leftBottomCorner),vec2(fragmentPos))){
                    
                    //compute surface normal using 2 corner points of inside and 1 corner point of outside
                    lightingNormal = calcPlaneNormal(leftBottomInside, leftCenterInside, leftBottomCorner);
                    
                    // fragment height
                    fragmentPos.z = getHeightOfPointOnSurface(vec2(fragmentPos), leftBottomCorner, lightingNormal);   
                }
                //bottom
                else if(pointLeftOfLine(vec2(leftBottomInside), vec2(rightBottomInside), vec2(fragmentPos))
                && pointLeftOfLine(vec2(rightBottomInside), vec2(rightBottomCorner), vec2(fragmentPos))){
                    //compute surface normal using 2 corner points of inside and 1 corner point of outside
                    lightingNormal = calcPlaneNormal(rightBottomInside, leftBottomInside, rightBottomCorner);

                    // fragment height
                    fragmentPos.z = getHeightOfPointOnSurface(vec2(fragmentPos), rightBottomCorner, lightingNormal);
                } 
                //right bottom
                else if(pointLeftOfLine(vec2(rightBottomInside),vec2(rightCenterInside), vec2(fragmentPos))
                && pointLeftOfLine(vec2(rightCenterInside),vec2(rightCenterCorner),vec2(fragmentPos))){

                    //compute surface normal using 2 corner points of inside and 1 corner point of outside
                    lightingNormal = calcPlaneNormal(rightCenterInside, rightBottomInside, rightCenterCorner);

                    // fragment height
                    fragmentPos.z = getHeightOfPointOnSurface(vec2(fragmentPos), rightCenterCorner, lightingNormal);
                }
                //right top
                else if(pointLeftOfLine(vec2(rightCenterInside),vec2(rightTopInside), vec2(fragmentPos))
                && pointLeftOfLine(vec2(rightTopInside),vec2(rightTopCorner),vec2(fragmentPos))
                ){

                    //compute surface normal using 2 corner points of inside and 1 corner point of outside
                    lightingNormal = calcPlaneNormal(rightTopInside, rightCenterInside, rightTopCorner);

                    // fragment height
                    fragmentPos.z = getHeightOfPointOnSurface(vec2(fragmentPos), rightTopCorner, lightingNormal);
                }
                //top
                else if(pointLeftOfLine(vec2(rightTopInside), vec2(leftTopInside), vec2(fragmentPos))){
                    
                    //compute surface normal using 2 corner points of inside and 1 corner point of outside
                    lightingNormal = calcPlaneNormal(leftTopInside, rightTopInside, leftTopCorner);

                    // fragment height
                    fragmentPos.z = getHeightOfPointOnSurface(vec2(fragmentPos), leftTopCorner, lightingNormal);
                }
            }
            //point is on the inside
            else{

                // fragemnt height
                fragmentPos.z = getHeightOfPointOnSurface(vec2(fragmentPos), tileCenter3D, lightingNormal);
            }
        }
        else{
             // fragemnt height
            fragmentPos.z = getHeightOfPointOnSurface(vec2(fragmentPos), tileCenter3D, lightingNormal);
            //debug
            //distance to center
            float distCenter = length(vec2(fragmentPos) - tileCenter2D); 
            float normDistCenter = mapInterval_O(distCenter, 0, int(ceil(tileSizeScreenSpace/2.0f)), 0.0f, 1.0f);
            float normZ = mapInterval_O(fragmentPos.z, 0, int(tileNormal.w), 0.0f, 1.0f);

         //hexTilesTexture = vec4(normZ, 0.0f, 0.0f, 1.0f);  
        //   hexTilesTexture = vec4(lightingNormal, 1.0f);
        }
    #endif

    
    // PHONG LIGHTING ----------------------------------------------------------------------------------------------

    hexTilesTexture.rgb = calculatePhongLighting(lightColor, lightPos, fragmentPos, lightingNormal, viewPos) * hexTilesTexture.rgb;
}
