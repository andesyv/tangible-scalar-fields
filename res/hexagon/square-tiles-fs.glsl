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
uniform sampler2D squareAccumulateTexture;

// 1D color map parameters
uniform sampler1D colorMapTexture;
uniform int textureWidth;

uniform int numberOfSamples;

//[x,y]
uniform vec2 maxBounds;
uniform vec2 minBounds;
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

    float maxBoundX = windowWidth/2*maxBoundNDC[0]+(windowWidth/2+maxBoundNDC[0]);
    float maxBoundY = windowHeight/2*maxBoundNDC[1]+(windowHeight/2+maxBoundNDC[1]);
    float minBoundX = windowWidth/2*minBoundNDC[0]+(windowWidth/2+minBoundNDC[0]);
    float minBoundY = windowHeight/2*minBoundNDC[1]+(windowHeight/2+minBoundNDC[1]);

    // to get intervals from 0 to maxTexCoord, we map the original Point interval to maxTexCoord+1
    // If the current value = maxValue, we take the maxTexCoord instead
    int squareX = min(maxTexCoord, mapInterval(gl_FragCoord.x, minBoundX, maxBoundX, maxTexCoord+1));
    int squareY = min(maxTexCoord, mapInterval(gl_FragCoord.y, minBoundY, maxBoundY, maxTexCoord+1));

    float squareValue = texelFetch(squareAccumulateTexture, ivec2(squareX, squareY), 0).r;

    //default color is all red
    squareTilesTexture = vec4(float(squareX/float(maxTexCoord)),float(squareY/float(maxTexCoord)),0.0f,1.0f);

	#ifdef COLORMAP

		//int colorTexelCoord = mapInterval(squareValue, 0, numberOfSamples, textureWidth);
		//squareTilesTexture.rgb = texelFetch(colorMapTexture, colorTexelCoord, 0).rgb;
	#endif
}
