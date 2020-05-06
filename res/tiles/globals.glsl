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
    vec3 specular = specularStrength * spec * lightColor;

    return (ambient + diffuse + specular);
}

//---------------------------------------------------------------------------------------------------------------

