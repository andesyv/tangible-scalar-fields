#version 450
#include "/defines.glsl"
#include "/globals.glsl"

//TODO: make changeable - ask thomas how
#define COLORMAP

//--in
layout(pixel_center_integer) in vec4 gl_FragCoord;

in vec2 maxBoundNDC;
in vec2 minBoundNDC;

//--uniform
uniform mat4 modelViewProjectionMatrix;

uniform sampler2D squareAccumulateTexture;

// 1D color map parameters
uniform sampler1D colorMapTexture;
uniform int textureWidth;

uniform int numberOfSamples;

//for now we assume texture is square maxX=maxY
//min = 0
uniform int maxTexCoord;

uniform int windowHeight;
uniform int windowWidth;
//--out
layout (location = 0) out vec4 squareTilesTexture;

//maps value x from [a,b] --> [0,c]
int mapInterval(float x, float a, float b, int c){
    return int((x-a)*c/(b-a));
}

void main()
{
    // get bounding box coordinates in Screen Space
    float maxBoundX = windowWidth/2*maxBoundNDC[0]+(windowWidth/2+maxBoundNDC[0]);
    float maxBoundY = windowHeight/2*maxBoundNDC[1]+(windowHeight/2+maxBoundNDC[1]);
    float minBoundX = windowWidth/2*minBoundNDC[0]+(windowWidth/2+minBoundNDC[0]);
    float minBoundY = windowHeight/2*minBoundNDC[1]+(windowHeight/2+minBoundNDC[1]);

    // to get intervals from 0 to maxTexCoord, we map the original Point interval to maxTexCoord+1
    // If the current value = maxValue, we take the maxTexCoord instead
    int squareX = min(maxTexCoord, mapInterval(gl_FragCoord.x, minBoundX, maxBoundX, maxTexCoord+1));
    int squareY = min(maxTexCoord, mapInterval(gl_FragCoord.y, minBoundY, maxBoundY, maxTexCoord+1));


    // convert square pos to screen space
    vec4 accPos = vec4(squareX, squareY, 0.0, 1.0);
    accPos = modelViewProjectionMatrix * accPos;
    accPos /= accPos.w;
    float squareXScreen = windowWidth/2*accPos[0]+(windowWidth/2+accPos[0]);
    float squareYScreen = windowHeight/2*accPos[1]+(windowHeight/2+accPos[1]);

    // get value from accumulate texture
    float squareValue = texelFetch(squareAccumulateTexture, ivec2(squareXScreen, squareYScreen), 0).r;

    //debug: color squares according to index
    squareTilesTexture = vec4(float(squareX/float(maxTexCoord)),float(squareY/float(maxTexCoord)),0.0f,1.0f);

	#ifdef COLORMAP

		int colorTexelCoord = mapInterval(squareValue, 0, numberOfSamples, textureWidth);
		squareTilesTexture.rgb = texelFetch(colorMapTexture, colorTexelCoord, 0).rgb;
	#endif
}
