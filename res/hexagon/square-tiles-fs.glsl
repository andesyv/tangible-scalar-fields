#version 450
#include "/defines.glsl"
#include "/globals.glsl"

//TODO: make changeable - ask thomas how
#define COLORMAP

//--in
layout(pixel_center_integer) in vec4 gl_FragCoord;

in vec4 boundsScreenSpace;
in vec2 origMaxBoundScreenSpace;

//--uniform
uniform mat4 modelViewProjectionMatrix;

uniform sampler2D squareAccumulateTexture;

// 1D color map parameters
uniform sampler1D colorMapTexture;
uniform int textureWidth;

uniform int numberOfSamples;

//min = 0
uniform int maxTexCoordX;
uniform int maxTexCoordY;
uniform int windowHeight;
uniform int windowWidth;
//--out
layout (location = 0) out vec4 squareTilesTexture;

//maps value x from [a,b] --> [0,c]
int mapInterval(float x, float a, float b, int c){
    return int((x-a)*c/(b-a));
}

// convert square pos to screen space
ivec2 getScreenSpaceTextureCoords(int squareX, int squareY){
    //LocalS
    vec4 accPos = vec4(squareX, squareY, 0.0, 1.0);
    //ClipS
    accPos = modelViewProjectionMatrix * accPos;
    //NDC
    accPos /= accPos.w;
    //Screen Space
    return ivec2(windowWidth/2 * accPos[0] + windowWidth/2, windowHeight/2 * accPos[1] + windowHeight/2);
}

void main()
{
    
    // to get intervals from 0 to maxTexCoord, we map the original Point interval to maxTexCoord+1
    // If the current value = maxValue, we take the maxTexCoord instead
    int squareX = min(maxTexCoordX, mapInterval(gl_FragCoord.x, boundsScreenSpace[2], boundsScreenSpace[0], maxTexCoordX+1));
    int squareY = min(maxTexCoordY, mapInterval(gl_FragCoord.y, boundsScreenSpace[3], boundsScreenSpace[1], maxTexCoordY+1));

    // get value from accumulate texture
    float squareValue = texelFetch(squareAccumulateTexture, getScreenSpaceTextureCoords(squareX, squareY), 0).r;

    // we don't want to render empty squares
    // we don't render pixels outisde the original bounding box
    if(squareValue < 0.00000001|| gl_FragCoord.x > origMaxBoundScreenSpace[0] || gl_FragCoord.y > origMaxBoundScreenSpace[1]){
        discard;
    }

    //debug: color squares according to index
    squareTilesTexture = vec4(float(squareX/float(maxTexCoordX)),float(squareY/float(maxTexCoordY)),0.0f,1.0f);

	#ifdef COLORMAP
		int colorTexelCoord = mapInterval(squareValue, 0, numberOfSamples, textureWidth);
		squareTilesTexture.rgb = texelFetch(colorMapTexture, colorTexelCoord, 0).rgb;
	#endif
}
