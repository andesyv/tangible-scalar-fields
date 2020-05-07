#include "/globals.glsl"

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
    if(modX != 2){
        if(modX == 0){
            if(modY == 0){
                //Upper Left
                a = ll;
                b = vec2(ll.x + rectWidth/2.0f, ll.y + rectHeight);
                if(!pointLeftOfLine(a,b,p)){
                    hexX--;
                }
            }
            //modY = 1
            else{
                //Lower Left
                a = vec2(ll.x + rectWidth/2.0f, ll.y);
                b = vec2(ll.x, ll.y + rectHeight);
                if(!pointLeftOfLine(a,b,p)){
                    hexX--;
                }
            }
        }
        //modX = 1
        else{

            if(modY == 0){
                //Upper Right
                a = vec2(ll.x + rectWidth/2.0f, ll.y + rectHeight);
                b = vec2(ll.x + rectWidth, ll.y);
                if(pointLeftOfLine(a,b,p)){
                    hexX--;
                }
            }
            //modY = 1
            else{
                //Lower Right
                a = vec2(ll.x + rectWidth, ll.y + rectHeight);
                b = vec2(ll.x + rectWidth/2.0f, ll.y);
                if(pointLeftOfLine(a,b,p)){
                    hexX--;
                }
            }
        }
    }
    else 
    {
		hexX--;
	}

    if(mod(hexX, 2) == 0){
        hexY = int((rectY-1)/2);
    }
    else{
        hexY = int(rectY/2);
    }

    return vec2(hexX, hexY);
}