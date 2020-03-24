

//maps value x from [a,b] --> [0,c]
int mapInterval(float x, float a, float b, int c){
    return int((x-a)*c/(b-a));
}

//maps value x from [a,b] --> [c,d]
float mapInterval_O(float x, int a, int b, float c, float d){
    return float((x-a)*(d-c)/float(b-a) + c);
}

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

// blends A over B
// https://de.wikipedia.org/wiki/Alpha_Blending
vec4 over(vec4 colA, vec4 colB){

    float alphaBlend = colA.a + (1-colA.a) * colB.a;
    return 1/alphaBlend * (colA.a*colA + (1-colA.a)*colB.a*colB);
}