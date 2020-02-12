#version 450
#include "/defines.glsl"

//TODO: make changeable - ask thomas how
#define COLORMAP

layout (location = 0) out vec4 pointChartTexture;

uniform vec3 pointColor;

// 1D color map parameters
uniform sampler1D colorMapTexture;
uniform int textureWidth;

uniform float viewportX;

void main()
{

	pointChartTexture = vec4(pointColor, 1.0f);

	#ifdef COLORMAP
		
		//simple 1D texel fetch by mapping the viewport.x to the colormap
		int texelCoord = int((gl_FragCoord.x * textureWidth)/viewportX);
		pointChartTexture.rgb = texelFetch(colorMapTexture, texelCoord, 0).rgb;
	#endif

}