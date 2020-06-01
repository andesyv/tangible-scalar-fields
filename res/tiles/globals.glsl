// BLENDING -----------------------------------------------------------------------------------------------------

// blends A over B
// https://de.wikipedia.org/wiki/Alpha_Blending
vec4 over(vec4 colA, vec4 colB){

    float alphaBlend = colA.a + (1-colA.a) * colB.a;
    return 1/alphaBlend * (colA.a*colA + (1-colA.a)*colB.a*colB);
}
//---------------------------------------------------------------------------------------------------------------

// INTERVAL MAPPING -----------------------------------------------------------------------------------------------------

//maps value x from [a,b] --> [0,c]
int mapInterval(float x, float a, float b, int c){
    return int((x-a)*c/(b-a));
}

//maps value x from [a,b] --> [c,d]
float mapInterval_O(float x, int a, int b, float c, float d){
    return float((x-a)*(d-c)/float(b-a) + c);
}

//---------------------------------------------------------------------------------------------------------------


// CONVERT TO SCREEN SPACE -----------------------------------------------------------------------------------------------------

vec2 getScreenSpacePosOfPoint(mat4 modelViewProjectionMatrix, vec2 coords, int windowWidth, int windowHeight){
    
    vec4 coordsNDC = modelViewProjectionMatrix * vec4(coords,0.0,1.0);

	vec2 coordsScreenSpace = vec2(windowWidth/2*coordsNDC[0]+windowWidth/2, //maxX
	windowHeight/2*coordsNDC[1]+windowHeight/2);//maxY

    return coordsScreenSpace;
}

vec4 getScreenSpacePosOfRect(mat4 modelViewProjectionMatrix, vec2 maxCoords, vec2 minCoords, int windowWidth, int windowHeight){
    
    vec4 boundsScreenSpace = vec4(
        getScreenSpacePosOfPoint(modelViewProjectionMatrix, maxCoords, windowWidth, windowHeight),
        getScreenSpacePosOfPoint(modelViewProjectionMatrix, minCoords, windowWidth, windowHeight)
    );

    return boundsScreenSpace;
}

// distance from size to size*2 in screen space 
vec2 getScreenSpaceSize(mat4 modelViewProjectionMatrix, vec2 size, int windowWidth, int windowHeight){
    return getScreenSpacePosOfPoint(modelViewProjectionMatrix, size * vec2(2,2), windowWidth, windowHeight)-getScreenSpacePosOfPoint(modelViewProjectionMatrix, size, windowWidth, windowHeight);
}

//---------------------------------------------------------------------------------------------------------------

// POINT SIDE LINE -----------------------------------------------------------------------------------------------------

bool pointLeftOfLine(vec2 a, vec2 b, vec2 p){
    return ((p.x-a.x)*(b.y-a.y)-(p.y-a.y)*(b.x-a.x)) > 0;
}

//---------------------------------------------------------------------------------------------------------------


// REGRESSION PLANE -----------------------------------------------------------------------------------------------------

//p1 = point needs z
//p2 = known point
//n = normal
// hessesche normal form: (p1.x-p2.x)*n.x + (p1.y-p2.y)*n.y + (p1.z-p2.z)*n.z = 0
// we have point p2 and the normal n
// we also have xy of point p1 and seach p1.z
// by solving for p1.z gives us the used formular
float getHeightOfPointOnSurface(vec2 p1, vec3 p2, vec3 n){
   return (p2.z*n.z - (p1.x-p2.x)*n.x + (p1.y-p2.y)*n.y)/n.z;
}

//https://stackoverflow.com/questions/5666222/3d-line-plane-intersection
//https://en.wikipedia.org/wiki/Line%E2%80%93plane_intersection 
// PRECONDITION: planeNormal and lineDir are normalized
vec3 linePlaneIntersection(vec3 planeNormal, vec3 planePoint, vec3 lineDir, vec3 linePoint){
    float t = dot(planeNormal, planePoint - linePoint) / dot(planeNormal, lineDir);        
    return linePoint + (lineDir * t);
}

//https://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line
float distancePointToLine(vec3 p, vec3 a, vec3 b){
    return abs((b.y-a.y)*p.x - (b.x-a.x)*p.y + b.x*a.y-b.y*a.x)
            /sqrt(pow((b.y-a.y),2) + pow((b.x-a.x),2));
}

vec3 calcPlaneNormal(vec3 a, vec3 b, vec3 c){

    vec3 aToB = b - a;
    vec3 aToC = c - a;
            
   return normalize(cross(aToB, aToC));
}

// https://stackoverflow.com/questions/49640250/calculate-normals-from-heightmap
vec3 calculateNormalFromHeightMap(ivec2 texelCoord, sampler2D texture){
                 
    // texelFetch (0,0) -> left bottom
    float top = texelFetch(texture, ivec2(texelCoord.x, texelCoord.y+1), 0).r;
    float bottom = texelFetch(texture, ivec2(texelCoord.x, texelCoord.y-1), 0).r;
    float left = texelFetch(texture, ivec2(texelCoord.x-1, texelCoord.y), 0).r;
    float right = texelFetch(texture, ivec2(texelCoord.x+1, texelCoord.y), 0).r;

    // reconstruct normal
    vec3 normal = vec3((left-right)/2,(bottom-top)/2, 1);
    return normalize(normal);
}
//---------------------------------------------------------------------------------------------------------------

// LIGHTING -----------------------------------------------------------------------------------------------------

vec3 calculatePhongLighting(vec3 lightColor, vec3 lightPos, vec3 fragmentPos, vec3 lightingNormal, vec3 viewPos){
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
    spec = max(0, spec);

    vec3 specular = specularStrength * spec * lightColor;

    return (ambient + diffuse + specular);
}

vec3 calculateBorderColor(vec3 fragmentPos, vec3 insideLightingNormal, vec3 outsideCornerA, vec3 insideCornerA, vec3 insideCornerB, 
                            vec3 tileCenter3D, float blendRange, vec4 tilesTexture, vec3 lightColor, vec3 lightPos, vec3 viewPos){
    
    //compute surface normal using 2 corner points of inside and 1 corner point of outside
    vec3 lightingNormalBorder = calcPlaneNormal(insideCornerA, insideCornerB, outsideCornerA);

    // fragment height in border
    fragmentPos.z = getHeightOfPointOnSurface(vec2(fragmentPos), outsideCornerA, lightingNormalBorder);
    
    // PHONG LIGHTING 
    vec3 borderColor = calculatePhongLighting(lightColor, lightPos, fragmentPos, lightingNormalBorder, viewPos) * tilesTexture.rgb;  

    // if the fragment is close to the border to the inside, we blend its color with the inside color to get rid of aliasing artifacts
    float distanceToTopLine = distancePointToLine(fragmentPos, insideCornerA, insideCornerB);
    if(distanceToTopLine < blendRange){
        
        float distanceToTopLineNorm = distanceToTopLine / blendRange;

        // fragment height if fragment would be on inside
        float insideHeight = getHeightOfPointOnSurface(vec2(fragmentPos), tileCenter3D, insideLightingNormal);    
        vec3 insideColor = calculatePhongLighting(lightColor, lightPos, vec3(vec2(fragmentPos), insideHeight), insideLightingNormal, viewPos) * tilesTexture.rgb;

        return distanceToTopLineNorm * borderColor + (1-distanceToTopLineNorm) * insideColor;
    }
    else{
        return borderColor;
    }   
}

//---------------------------------------------------------------------------------------------------------------

