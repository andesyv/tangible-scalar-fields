

//maps value x from [a,b] --> [0,c]
int mapInterval(float x, float a, float b, int c){
    return int((x-a)*c/(b-a));
}

//maps value x from [a,b] --> [c,d]
float mapInterval_O(float x, int a, int b, float c, float d){
    return float((x-a)*(d-c)/float(b-a) + c);
}

vec4 getScreenSpacePosOfRect(mat4 modelViewProjectionMatrix, vec2 maxCoords, vec2 minCoords, int windowWidth, int windowHeight){
    
    //calc rect coordinates in NDC Space
    vec4 maxCoordsNDC = modelViewProjectionMatrix * vec4(maxCoords,0.0,1.0);
    vec4 minCoordsNDC = modelViewProjectionMatrix * vec4(minCoords,0.0,1.0);

	// get rect coordinates in Screen Space
    vec4 boundsScreenSpace = vec4(windowWidth/2*maxCoordsNDC[0]+windowWidth/2, //maxX
	windowHeight/2*maxCoordsNDC[1]+windowHeight/2, //maxY
	windowWidth/2*minCoordsNDC[0]+windowWidth/2, //minX
	windowHeight/2*minCoordsNDC[1]+windowHeight/2); //minY

    return boundsScreenSpace;
}