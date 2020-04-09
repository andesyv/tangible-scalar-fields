#version 450
#extension GL_ARB_shading_language_include : require
#include "/defines.glsl"
#include "/globals.glsl"

layout(location = 0) in float xValue;
layout(location = 1) in float yValue;

//[x,y]
uniform vec2 maxBounds_Off;

uniform vec2 maxBounds_rect;
uniform vec2 minBounds;
//min = 0
uniform int maxTexCoordX;
uniform int maxTexCoordY;

uniform int max_rect_col;
uniform int max_rect_row;

uniform float rectHeight;
uniform float rectWidth;

out vec4 vertexColor;

void main()
{

    vec2 hex = matchPointWithHexagon(vec2 (xValue, yValue), max_rect_col, max_rect_row, rectWidth, rectHeight, minBounds, maxBounds_rect);
    /*vec2 hex;
    // to get intervals from 0 to maxCoord, we map the original Point interval to maxCoord+1
    // If the current value = maxValue, we take the maxCoord instead
    int rectX = min(max_rect_col, mapInterval(xValue, minBounds[0], maxBounds_rect[0], max_rect_col+1));
    int rectY = min(max_rect_row, mapInterval(yValue, minBounds[1], maxBounds_rect[1], max_rect_row+1));

    // rectangle left lower corner in space of points
    vec2 ll = vec2(rectX * rectWidth + minBounds.x , rectY * rectHeight + minBounds.y);
    vec2 p = vec2(xValue, yValue);
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
                if(pointOutsideHex(b,a,p)){
                    hexX--;
                }
            }
            //modY = 1
            else{
                //Lower Left
                a = vec2(ll.x + rectWidth/2.0f, ll.y);
                b = vec2(ll.x, ll.y + rectHeight);
                if(pointOutsideHex(b,a,p)){
                    hexX--;
                }
            }
        }
        //modX = 2
        else{

            if(modY == 0){
                //Upper Right
                a = vec2(ll.x + rectWidth, ll.y);
                b = vec2(ll.x + rectWidth/2.0f, ll.y + rectHeight);
                if(pointOutsideHex(a,b,p)){
                    hexX++;
                }
            }
            //modY = 1
            else{
                //Lower Right
                a = vec2(ll.x + rectWidth/2.0f, ll.y);
                b = vec2(ll.x + rectWidth, ll.y + rectHeight);
                if(pointOutsideHex(a,b,p)){
                    hexX++;
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

    hex = vec2(hexX, hexY);*/

    // we only set the red channel, because we only use the color for additive blending
    vertexColor = vec4(1.0f,0.0f,0.0f,1.0f);

    // debug: color point according to square
   // vertexColor = vec4(float(hex.x/float(maxTexCoordX)),float(hex.y/float(maxTexCoordY)),0.0f,1.0f);

    // NDC - map square coordinates to [-1,1] (first [0,2] than -1)
	//vec2 rectNDC = vec2(((rectX * 2) / float(max_rect_col+1)) - 1, ((rectY * 2) / float(max_rect_row+1)) - 1);

    vec2 hexNDC = vec2(((hex.x * 2) / float(maxTexCoordX + 1)) - 1, ((hex.y * 2) / float(maxTexCoordY + 1)) - 1);

    gl_Position = vec4(hexNDC, 0.0f, 1.0f);
}