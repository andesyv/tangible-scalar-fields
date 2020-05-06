#version 450
#include "/defines.glsl"
#include "/globals.glsl"

//--in
layout(pixel_center_integer) in vec4 gl_FragCoord;

layout(std430, binding = 0) buffer tileNormalsBuffer
{
    uint tileNormals[];
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

//Lighting----------------------
uniform vec3 lightPos; 
uniform vec3 viewPos; //is vec3(0,0,0) because we work in viewspace
uniform vec3 lightColor;
//----------------------

//--out
layout (location = 0) out vec4 squareTilesTexture;

bool pointInBorder(vec2 p){
    return !((p.x >= insideBoundingBox.left && p.x <= insideBoundingBox.right) &&
    (p.y <= insideBoundingBox.top && p.y >= insideBoundingBox.bottom));
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
    vec3 lightingNormal = vec3(0.0f,0.0f,0.0f);
    vec3 fragmentPos = vec3(gl_FragCoord);
    #ifdef RENDER_TILE_NORMALS
        for(int i = 0; i < 4; i++){
            tileNormal[i] = float(tileNormals[(squareX*(maxTexCoordY+1) + squareY) * 4 + i]);
        }
        tileNormal /= normalsFactor;
       
        //Corner Of Square (z=0)
        vec3 leftBottomCorner = vec3(squareX * tileSizeScreenSpace + boundsScreenSpace[2], squareY * tileSizeScreenSpace + boundsScreenSpace[3], 0.0f);
        vec3 leftTopCorner = vec3(leftBottomCorner.x, leftBottomCorner.y+tileSizeScreenSpace, 0.0f);
        vec3 rightBottomCorner = vec3(leftBottomCorner.x + tileSizeScreenSpace, leftBottomCorner.y, 0.0f);
        vec3 rightTopCorner = vec3(rightBottomCorner.x, rightBottomCorner.y + tileSizeScreenSpace, 0.0f);
        // move size/2 up and right
        vec2 tileCenter2D = vec2(leftBottomCorner) + tileSizeScreenSpace / 2.0f;
        //height at tile center
        float tileCenterZ = tileNormal.z * tileHeightMult;
   
        vec3 tileCenter3D = vec3(tileCenter2D, tileCenterZ);

        // LIGHTING NORMAL ------------------------
        //to debug normals set z ~= 0.01f
        lightingNormal = vec3(tileNormal.x/tileNormal.w, tileNormal.y/tileNormal.w, 1.0f);
        lightingNormal = normalize(lightingNormal);
        //-----------------------------------------

        // BORDER-------------------------------------
       
        float insideSizeFromCenter = (1.0f - borderWidth) * tileSizeScreenSpace/2.0f;
        
        insideBoundingBox.left = tileCenter2D.x - insideSizeFromCenter;
        insideBoundingBox.right = tileCenter2D.x + insideSizeFromCenter;
        insideBoundingBox.top = tileCenter2D.y + insideSizeFromCenter;
        insideBoundingBox.bottom = tileCenter2D.y - insideSizeFromCenter;

        //get height of inside bounding box points
        vec3 leftBottomInside = vec3(insideBoundingBox.left, insideBoundingBox.bottom, getHeightOfPointOnSurface(vec2(insideBoundingBox.left, insideBoundingBox.bottom),tileCenter3D,lightingNormal));  
        vec3 leftTopInside = vec3(insideBoundingBox.left, insideBoundingBox.top, getHeightOfPointOnSurface(vec2(insideBoundingBox.left, insideBoundingBox.top),tileCenter3D,lightingNormal));
        vec3 rightBottomInside = vec3(insideBoundingBox.right, insideBoundingBox.bottom, getHeightOfPointOnSurface(vec2(insideBoundingBox.right, insideBoundingBox.bottom),tileCenter3D,lightingNormal));  
        vec3 rightTopInside = vec3(insideBoundingBox.right, insideBoundingBox.top, getHeightOfPointOnSurface(vec2(insideBoundingBox.right, insideBoundingBox.top),tileCenter3D,lightingNormal));  

        //--------------------------------------------

        if(pointInBorder(vec2(fragmentPos))){
            //check which side
            //left
            if(fragmentPos.x < insideBoundingBox.left && 
            (fragmentPos.y < insideBoundingBox.top || (fragmentPos.y > insideBoundingBox.top && abs(fragmentPos.x - insideBoundingBox.left) > abs(fragmentPos.y - insideBoundingBox.top))) && 
            (fragmentPos.y > insideBoundingBox.bottom || (fragmentPos.y < insideBoundingBox.bottom && abs(fragmentPos.x - insideBoundingBox.left) > abs(fragmentPos.y - insideBoundingBox.bottom)))){
            
                //compute surface normal using 2 corner points of inside and 1 corner point of outside
                lightingNormal = calcPlaneNormal(leftBottomInside, leftTopInside, leftBottomCorner);
                
                // fragment height
                fragmentPos.z = getHeightOfPointOnSurface(vec2(fragmentPos), leftBottomCorner, lightingNormal);

            }
            //right
            else if(fragmentPos.x > insideBoundingBox.right && 
            (fragmentPos.y < insideBoundingBox.top || (fragmentPos.y > insideBoundingBox.top && abs(fragmentPos.x - insideBoundingBox.right) > abs(fragmentPos.y - insideBoundingBox.top))) && 
            (fragmentPos.y > insideBoundingBox.bottom || (fragmentPos.y < insideBoundingBox.bottom && abs(fragmentPos.x - insideBoundingBox.right) > abs(fragmentPos.y - insideBoundingBox.bottom)))){

                //compute surface normal using 2 corner points of inside and 1 corner point of outside
                lightingNormal = calcPlaneNormal(rightBottomInside, rightTopInside, rightBottomCorner);

                // fragment height
                fragmentPos.z = getHeightOfPointOnSurface(vec2(fragmentPos), rightBottomCorner, lightingNormal);
            }
            //bottom
            else if(fragmentPos.y < insideBoundingBox.bottom){
                //compute surface normal using 2 corner points of inside and 1 corner point of outside
                lightingNormal = calcPlaneNormal(leftBottomInside, rightBottomInside, leftBottomCorner);

                // fragment height
                fragmentPos.z = getHeightOfPointOnSurface(vec2(fragmentPos), leftBottomCorner, lightingNormal);
            }
            //top
            else if(fragmentPos.y > insideBoundingBox.top){
                
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

            //debug
            //distance to center
            float distCenter = length(vec2(fragmentPos) - tileCenter2D); 
            float normDistCenter = mapInterval_O(distCenter, 0, int(ceil(tileSizeScreenSpace/2.0f)), 0.0f, 1.0f);
            float normZ = mapInterval_O(fragmentPos.z, 0, int(tileNormal.w), 0.0f, 1.0f);

           // squareTilesTexture = vec4(normZ, 0.0f, 0.0f, 1.0f);  
        // squareTilesTexture = vec4(lightingNormal, 1.0f);
        }
    #endif

    // PHONG LIGHTING ----------------------------------------------------------------------------------------------
    // TODO: move to global.glsl
    //TODO: maybe create different global.glsl for different groups of methods
    // ambient
    float ambientStrength = 0.8;
    vec3 ambient = ambientStrength * lightColor;
  	
    // diffuse 
    vec3 lightDir = normalize(lightPos - fragmentPos);
    float diff = max(dot(lightingNormal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - fragmentPos);
    vec3 reflectDir = reflect(-lightDir, lightingNormal);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;  
        
    squareTilesTexture.rgb = (ambient + diffuse + specular) * squareTilesTexture.rgb;
}
