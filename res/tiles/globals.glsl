

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

bool pointOutsideHex(vec2 a, vec2 b, vec2 p){
    return ((p.x-a.x)*(b.y-a.y)-(p.y-a.y)*(b.x-a.x)) < 0;
}

vec2 matchPointWithHexagon(vec2 p, int max_rect_col,int max_rect_row, float rectWidth, float rectHeight, vec2 minBounds, vec2 maxBounds){
    
    // to get intervals from 0 to maxCoord, we map the original Point interval to maxCoord+1
    // If the current value = maxValue, we take the maxCoord instead
    int rectX = min(max_rect_col, mapInterval(p.x, minBounds.x, maxBounds.x, max_rect_col+1));
    int rectY = min(max_rect_row, mapInterval(p.y, minBounds.y, maxBounds.y, max_rect_row+1));

    // rectangle left lower corner in space of points
    vec2 ll = vec2(rectX * rectWidth + minBounds.x, rectY * rectHeight + minBounds.y);
    vec2 a, b;

    // calculate hexagon index from rectangle index
    int hexX, hexY, modX, modY;
    
    // get modulo values
    modX = int(mod(rectX,3));
    modY = int(mod(rectY,2));

    //calculate X index
    hexX = int(rectX/3) * 2 + modX;
    if(modX != 0){
        if(modX == 1){
            if(modY == 0){
                //Upper Left
                a = ll;
                b = vec2(ll.x + rectWidth/2.0f, ll.y + rectHeight);
                if(pointOutsideHex(a,b,p)){
                    hexX--;
                }
            }
            //modY = 1
            else{
                //Lower Left
                a = vec2(ll.x + rectWidth/2.0f, ll.y);
                b = vec2(ll.x, ll.y + rectHeight);
                if(pointOutsideHex(a,b,p)){
                    hexX--;
                }
            }
        }
        //modX = 2
        else{

            if(modY == 0){
                //Upper Right
                a = vec2(ll.x + rectWidth/2.0f, ll.y + rectHeight);
                b = vec2(ll.x + rectWidth, ll.y);
                if(!pointOutsideHex(a,b,p)){
                    hexX--;
                }
            }
            //modY = 1
            else{
                //Lower Right
                a = vec2(ll.x + rectWidth, ll.y + rectHeight);
                b = vec2(ll.x + rectWidth/2.0f, ll.y);
                if(!pointOutsideHex(a,b,p)){
                    hexX--;
                }
            }
        }
    }

    if(mod(hexX, 2) == 0){
        hexY = int(rectY/2);
    }
    else{
        hexY = int((rectY+1)/2);
    }

    return vec2(hexX, hexY);
}

