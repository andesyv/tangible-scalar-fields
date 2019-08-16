#version 450
#include "/defines.glsl"
#include "/globals.glsl"

layout(pixel_center_integer) in vec4 gl_FragCoord;

uniform mat4 modelViewMatrix;
uniform mat4 modelViewProjectionMatrix;
uniform mat4 inverseModelViewProjectionMatrix;
uniform mat3 normalMatrix;
uniform mat3 inverseNormalMatrix;

uniform vec3 lightPosition;
uniform vec3 diffuseMaterial;
uniform vec3 ambientMaterial;
uniform vec3 specularMaterial;
uniform float shininess;

// GUI dependent color selection
uniform vec3 samplePointColor;
uniform vec3 backgroundColor;
uniform vec3 contourLineColor;
uniform vec3 lensBorderColor;

uniform sampler2D surfacePositionTexture;
uniform sampler2D surfaceNormalTexture;

uniform sampler2D depthTexture;
uniform sampler2D ambientTexture;

// texture for blending surface and classical scatter plot
uniform sampler2D scatterPlotTexture;
uniform float opacityScale;

// KDE texture
uniform sampler2D kernelDensityTexture;

// focus and context
uniform vec2 focusPosition;
uniform float lensSize;
uniform float aspectRatio;
uniform float windowHeight;
uniform float windowWidth;

// 1D color map parameters
uniform sampler1D colorMapTexture;
uniform int textureWidth;

const int rightDistance = 50;
const int topDistance = 50;
const int guiWidth = 40;

// contour lines
uniform int contourCount;
uniform float contourThickness;	

in vec4 gFragmentPosition;
out vec4 fragColor;

layout(std430, binding = 3) buffer depthRangeBuffer
{
	uint minDepth;
	uint maxDepth;
};

// See Porter-Duff 'A over B' operators: https://de.wikipedia.org/wiki/Alpha_Blending
vec4 overOperator(vec4 source, vec4 destination)
{
	float ac = source.a + (1.0f - source.a) * destination.a;
	vec4 blended = 1.0f/ac * (source.a * source + (1.0f - source.a) * destination.a * destination);

	// make sure dimensions fit after blending 
	return clamp(blended, vec4(0), vec4(1));
}

void main()
{
	vec4 fragCoord = gFragmentPosition;
	fragCoord /= fragCoord.w;
	
	vec4 near = inverseModelViewProjectionMatrix*vec4(fragCoord.xy,-1.0,1.0);
	near /= near.w;

	vec4 far = inverseModelViewProjectionMatrix*vec4(fragCoord.xy,1.0,1.0);
	far /= far.w;

	vec3 V = normalize(far.xyz-near.xyz);

	vec4 surfacePosition = texelFetch(surfacePositionTexture,ivec2(gl_FragCoord.xy),0);
	vec4 surfaceNormal = texelFetch(surfaceNormalTexture,ivec2(gl_FragCoord.xy),0);
	vec3 surfaceNormalWorld = normalize(inverseNormalMatrix*surfaceNormal.xyz);

	float depth = texelFetch(depthTexture,ivec2(gl_FragCoord.xy),0).x;
	vec4 ambient = vec4(1.0f, 1.0f, 1.0f, 1.0f);

#ifdef AMBIENT
	ambient = texelFetch(ambientTexture,ivec2(gl_FragCoord.xy),0);
#endif

	// set background color as default fragment color
	vec4 finalColor = vec4(backgroundColor, 0.0f);

	// read texel from scatterplot texture
	vec4 scatterPlot = texelFetch(scatterPlotTexture, ivec2(gl_FragCoord.xy), 0).rgba;

	// convert from CMY to RGB color-space
	scatterPlot.rgb = vec3(1.0f) - scatterPlot.rgb;


	// 1D textur dimensions
	int newDepthMin = 0;
	int newDepthMax = textureWidth-1;

	// calculate 'Point-Plane Distance' of a fragment (using the camera view-vector as plane normal)
	vec4 posViewSpace = modelViewMatrix*surfacePosition;
	float oldDepthValue = abs(dot(posViewSpace.xyz,V));

	// read from SSBO and convert back to float: 
	// - https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/intBitsToFloat.xhtml
	float oldDepthMin =  uintBitsToFloat(minDepth);
	float oldDepthMax =  uintBitsToFloat(maxDepth);

	// depth range
	float oldDepthRange = (oldDepthMax - oldDepthMin);  
	float newDepthRange = (newDepthMax - newDepthMin);  		

	// convert to different range: 
	// - https://stackoverflow.com/questions/929103/convert-a-number-range-to-another-range-maintaining-ratio
	float newDepthValue = (((oldDepthValue - oldDepthMin) * newDepthRange) / oldDepthRange) + newDepthMin;
	
	if(depth != 1.0f)
	{	
		// assign default color and alpha
		finalColor = vec4(samplePointColor, 1.0f);

	#ifdef SCATTERPLOT
		finalColor.rgb = scatterPlot.rgb;

		// clamp alpha to 1.0f to be able to blend it with the background
		finalColor.a = min(scatterPlot.a, 1.0f);
	#endif

	#ifdef COLORMAP
		// apply color map  using texelFetch with a range from [0, size) 
		finalColor.rgb = texelFetch(colorMapTexture, newDepthMax-int(round(newDepthValue)), 0).rgb;

		#ifdef SCATTERPLOT
			finalColor.rgb *= scatterPlot.rgb;
		#endif
	#endif
	}


	float light_occlusion = 1.0;

#ifdef AMBIENT
	vec3 VL =  normalize(normalMatrix*normalize(lightPosition-surfacePosition.xyz));
	light_occlusion = 1.0f-clamp(dot(VL.xyz, ambient.xyz),0.0,1.0); 
#endif

#ifdef ILLUMINATION
	vec3 ambientColor = finalColor.rgb*ambientMaterial*ambient.a;
	vec3 diffuseColor = finalColor.rgb*diffuseMaterial;
	vec3 specularColor = finalColor.rgb*specularMaterial;

	vec3 N = surfaceNormalWorld;
	vec3 L = normalize(lightPosition-surfacePosition.xyz);
	vec3 R = normalize(reflect(L, N));

	float NdotL = dot(N, L);
	float RdotV = max(0.0,dot(R, V));

	NdotL = clamp((NdotL+1.0)*0.5,0.0,1.0);
	finalColor.rgb = ambientColor + light_occlusion * (NdotL * diffuseColor + pow(RdotV,shininess) * specularColor);
#endif


// Curvature based blending -------------------------------------------------------------------------------------------------------
#ifdef CURVEBLENDING	
	
	// texelFetch (0,0) -> left bottom
	vec3 top = texelFetch(surfaceNormalTexture, ivec2(gl_FragCoord.x, gl_FragCoord.y+1), 0).xyz;
	vec3 bottom = texelFetch(surfaceNormalTexture, ivec2(gl_FragCoord.x, gl_FragCoord.y-1), 0).xyz;
	vec3 left = texelFetch(surfaceNormalTexture, ivec2(gl_FragCoord.x-1, gl_FragCoord.y), 0).xyz;
	vec3 right = texelFetch(surfaceNormalTexture, ivec2(gl_FragCoord.x+1, gl_FragCoord.y), 0).xyz;
	
	// compute curvature dependent opacity
	float opacity = length(top-surfaceNormal.xyz) + length(bottom-surfaceNormal.xyz) + length(left-surfaceNormal.xyz) + length(right-surfaceNormal.xyz);
	
	// transform to [0,1];
	opacity /= 4 * sqrt(2);

	// scale opacity
	opacity = pow(opacity, opacityScale);

	vec3 blendColor = backgroundColor;

	// emphasize scatterplot and allow changes with the GUI
	if(scatterPlot.a > 0.0f)
	{
		blendColor = scatterPlot.rgb*samplePointColor;
		blendColor.rgb *= (0.5+0.5*light_occlusion*ambient.a);
	}

	// apply over-operator
	finalColor.rgb = overOperator(vec4(finalColor.rgb, opacity), vec4(blendColor.rgb, 1.0f-opacity)).rgb;
#endif

// Distance based blending --------------------------------------------------------------------------------------------------------
#ifdef DISTANCEBLENDING

	// convert to range [0,1]:
	float opacity = max(1-(newDepthValue/newDepthRange), 0.0f);

	// scale opacity
	opacity = pow(opacity, opacityScale);

	vec3 blendColor = backgroundColor;

	// emphasize scatterplot and allow changes with the GUI
	if(scatterPlot.a > 0.0f)
	{
		blendColor = scatterPlot.rgb*samplePointColor;
		blendColor.rgb *= (0.5+0.5*light_occlusion*ambient.a);
	}

	// apply over-operator
	finalColor.rgb = overOperator(vec4(finalColor.rgb, opacity), vec4(blendColor.rgb, 1.0f-opacity)).rgb;
#endif

// Normal based blending ----------------------------------------------------------------------------------------------------------
#ifdef NORMALBLENDING

	// emphasize the curvature
	float opacity = 1.0f - dot(surfaceNormal.xyz, V);

	// scale opacity and apply to alpha
	opacity = pow(opacity, opacityScale);

	vec3 blendColor = backgroundColor;

	// emphasize scatterplot and allow changes with the GUI
	if(scatterPlot.a > 0.0f)
	{
		blendColor = scatterPlot.rgb*samplePointColor;
		blendColor.rgb *= (0.5+0.5*light_occlusion*ambient.a);
	}

	// apply over-operator
	finalColor.rgb = overOperator(vec4(finalColor.rgb, opacity), vec4(blendColor.rgb, 1.0f-opacity)).rgb;
#endif
//---------------------------------------------------------------------------------------------------------------------------------


	// blend with background color
	finalColor = overOperator(finalColor, vec4(backgroundColor, 1.0f-finalColor.a));


#ifdef CONTOURLINES
	// normalize depth-range to [0, 1]
	float normalizedTexturePos = 1.0f - ((oldDepthValue-oldDepthMin) / oldDepthRange);
	float discreticedPos = mod(normalizedTexturePos, 1.0f/(contourCount+1.0f));

	if (discreticedPos <= contourThickness && normalizedTexturePos >= contourThickness && depth != 1.0f)
	{
		// assign contour line color
		finalColor.rgb = contourLineColor;
		finalColor.a = 1.0f;
	}
#endif

#ifdef LENSING
	float pxlDistance = length((focusPosition-gFragmentPosition.xy) / vec2(aspectRatio, 1.0));
	
	// anti-aliasing distance -> make lens thickness independent of window size
	float startInner = lensSize - 7.5f /*empirically chosen*/ / windowHeight;
	float endOuter = lensSize + 7.5f /*empirically chosen*/ / windowHeight;

	// draw border of lens
	if(pxlDistance >= startInner && pxlDistance <= lensSize)
	{
		finalColor.rgb = mix(finalColor.rgb, lensBorderColor, smoothstep(startInner, lensSize, pxlDistance));
	}
	else if (pxlDistance > lensSize && pxlDistance <= endOuter)
	{
		finalColor.rgb = mix(finalColor.rgb, lensBorderColor, 1.0f - smoothstep(lensSize, endOuter, pxlDistance));
	}
#endif
	
#ifdef HEATMAP_GUI
	if(gl_FragCoord.x <= windowWidth-rightDistance && gl_FragCoord.x > windowWidth-rightDistance-guiWidth && gl_FragCoord.y < windowHeight-topDistance && gl_FragCoord.y >= windowHeight-topDistance-textureWidth) 
	{	
		int texCoords = textureWidth-int(windowHeight-topDistance-gl_FragCoord.y);
		finalColor.rgb = texelFetch(colorMapTexture, texCoords, 0).rgb;
	}
#endif

	fragColor = finalColor; 
}