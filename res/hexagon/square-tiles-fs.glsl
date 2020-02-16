#version 450
#include "/defines.glsl"
#include "/globals.glsl"

//TODO: make changeable - ask thomas how
#define COLORMAP

//--in
layout(pixel_center_integer) in vec4 gl_FragCoord;

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

//--out
layout (location = 1) out vec4 squareTilesTexture;

//maps value x from [a,b] --> [0,c]
int mapInterval(float x, float a, float b, int c){
    return int((x-a)*c/(b-a));
}

void main()
{

    // to get intervals from 0 to maxTexCoord, we map the original Point interval to maxTexCoord+1
    // If the current value = maxValue, we take the maxTexCoord instead
    int squareX = min(maxTexCoord, mapInterval(gl_FragCoord.x, minBounds[0], maxBounds[0], maxTexCoord+1));
    int squareY = min(maxTexCoord, mapInterval(gl_FragCoord.y, minBounds[1], maxBounds[1], maxTexCoord+1));

    float squareValue = texelFetch(squareAccumulateTexture, ivec2(squareX, squareY), 0).r;

    //default color is all red
    squareTilesTexture = vec4(1.0f,0.0f,0.0f,1.0f);

	#ifdef COLORMAP

		int colorTexelCoord = mapInterval(squareValue, 0, numberOfSamples, textureWidth);
		squareTilesTexture.rgb = texelFetch(colorMapTexture, colorTexelCoord, 0).rgb;
	#endif
}
