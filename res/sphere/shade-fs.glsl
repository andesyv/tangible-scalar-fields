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

uniform sampler2D sphereDiffuseTexture;
uniform sampler2D surfacePositionTexture;
uniform sampler2D surfaceNormalTexture;
uniform sampler2D surfaceDiffuseTexture;
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

// 1D color map parameters
uniform sampler1D colorMapTexture;
uniform int textureWidth;

// contour lines
uniform int contourCount;
uniform float contourThickness;	

// color constants
const vec3 contourColor = vec3(0.9f, 0.09f, 0.05f);
const vec3 lensBorderColor = vec3(0.9f, 0.09f, 0.05f);

in vec4 gFragmentPosition;
out vec4 fragColor;

layout(std430, binding = 3) buffer depthRangeBuffer
{
	uint minDepth;
	uint maxDepth;

	uint minKernelDifference;
	uint maxKernelDifference;

	uint maxScatterPlotAlpha;
};

// From http://http.developer.nvidia.com/GPUGems/gpugems_ch17.html
vec2 latlong(vec3 v)
{
	v = normalize(v);
	float theta = acos(v.z) / 2; // +z is up
	float phi = atan(v.y, v.x) + 3.1415926535897932384626433832795;
	return vec2(phi, theta) * vec2(0.1591549, 0.6366198);
}

vec4 over(vec4 vecF, vec4 vecB)
{
	return vecF + (1.0-vecF.a)*vecB;
}

float aastep(float threshold, float value) {
  #ifdef GL_OES_standard_derivatives
    float afwidth = length(vec2(dFdx(value), dFdy(value))) * 0.70710678118654757;
    return smoothstep(threshold-afwidth, threshold+afwidth, value);
  #else
    return step(threshold, value);
  #endif  
}

// See Porter-Duff 'A over B' operators: https://de.wikipedia.org/wiki/Alpha_Blending
vec4 overOperator(vec4 source, vec4 destination)
{
	float ac = source.a + (1.0f - source.a) * destination.a;
	vec4 blended = 1.0f/ac * (source.a * source + (1.0f - source.a) * destination.a * destination);

	// make sure dimensions fit after blending 
	blended = clamp(blended, vec4(0), vec4(1));

	return blended;
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

	//vec4 spherePosition = texelFetch(spherePositionTexture,ivec2(gl_FragCoord.xy),0);
	//vec4 sphereNormal = texelFetch(sphereNormalTexture,ivec2(gl_FragCoord.xy),0);
	vec4 sphereDiffuse = texelFetch(sphereDiffuseTexture,ivec2(gl_FragCoord.xy),0);

	vec4 surfacePosition = texelFetch(surfacePositionTexture,ivec2(gl_FragCoord.xy),0);
	vec4 surfaceNormal = texelFetch(surfaceNormalTexture,ivec2(gl_FragCoord.xy),0);
	vec4 surfaceDiffuse = texelFetch(surfaceDiffuseTexture,ivec2(gl_FragCoord.xy),0);
	
	if (surfacePosition.w >= 65535.0)	
		surfaceDiffuse.a = 0.0;

	vec4 ambient = vec4(1.0,1.0,1.0,1.0);

#ifdef AMBIENT
	ambient = texelFetch(ambientTexture,ivec2(gl_FragCoord.xy),0);
#endif

	float depth = texelFetch(depthTexture,ivec2(gl_FragCoord.xy),0).x;
	vec3 surfaceNormalWorld = normalize(inverseNormalMatrix*surfaceNormal.xyz);

	// read texel from scatterplot texture
	vec4 scatterPlot = texelFetch(scatterPlotTexture, ivec2(gl_FragCoord.xy), 0).rgba;

	// normalize scatterplot alpha range to [0,1]
	scatterPlot.a /= uintBitsToFloat(maxScatterPlotAlpha);

	if(depth != 1.0f)
	{
	
		// set default vaule in case no color-map texture was bound
		vec3 colorMapTexel = vec3(1,1,1);

		// 1D textur dimensions
		int newMin = 0;
		int newMax = textureWidth-1;

		// calculate 'Point-Plane Distance' of a fragment (using the camera view-vector as plane normal)
		vec4 posViewSpace = modelViewMatrix*surfacePosition;
		float oldValue = abs(dot(posViewSpace.xyz,V));

		// read from SSBO and convert back to float: 
		// - https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/intBitsToFloat.xhtml
		float oldMin =  uintBitsToFloat(minDepth);
		float oldMax =  uintBitsToFloat(maxDepth);

		// depth range
		float oldRange = (oldMax - oldMin);  
		float newRange = (newMax - newMin);  		

		// convert to different range: 
		// - https://stackoverflow.com/questions/929103/convert-a-number-range-to-another-range-maintaining-ratio
		float newValue = (((oldValue - oldMin) * newRange) / oldRange) + newMin;
	
	#ifdef COLORMAP
		// apply color map  using texelFetch with a range from [0, size) 
		colorMapTexel = texelFetch(colorMapTexture, newMax-int(round(newValue)), 0).rgb; 
		surfaceDiffuse.rgb  = colorMapTexel; 
	#endif

	#ifdef SCATTERPLOT
		surfaceDiffuse.rgb = colorMapTexel * scatterPlot.rgb;
	#endif

	#ifdef CONTOURLINES
		// create contour-lines
		float normalizedTexturePos = 1.0f - ((oldValue-oldMin) / oldRange);

		float discreticedPos = mod(normalizedTexturePos, 1.0f/(contourCount+1.0f));

		if (discreticedPos <= contourThickness && normalizedTexturePos >= contourThickness)
		{
			// assign contour line color
			surfaceDiffuse.rgb = vec3(1.0f, 0.0f, 0.0f);
		}
	#endif
	}

	vec3 color = surfaceDiffuse.rgb;
	float light_occlusion = 1.0;

#ifdef AMBIENT
	vec3 VL =  normalize(normalMatrix*normalize(lightPosition-surfacePosition.xyz));
	light_occlusion = 1.0-clamp(dot(-VL.xyz, ambient.xyz),0.0,1.0); // negation of VL is a hack -- maybe surfacePosition.z wrong?
#endif

#ifdef ILLUMINATION
	vec3 ambientColor = surfaceDiffuse.rgb*ambientMaterial*ambient.a;
	vec3 diffuseColor = surfaceDiffuse.rgb*diffuseMaterial;
	vec3 specularColor = surfaceDiffuse.rgb*specularMaterial;

	vec3 N = surfaceNormalWorld;
	vec3 L = normalize(lightPosition-surfacePosition.xyz);
	vec3 R = normalize(reflect(L, N));
	float NdotV = max(0.0,abs(dot(N, V)));
	float NdotL = dot(N, L);
	float RdotV = max(0.0,dot(R, V));

	NdotL = clamp((NdotL+1.0)*0.5,0.0,1.0);
	color = ambientColor + light_occlusion * (NdotL * diffuseColor + pow(RdotV,shininess) * specularColor);
#endif

	vec4 final = vec4(color.rgb,surfaceDiffuse.a);

// Curvature based blending 
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

	// scale opacity and apply to alpha
	opacity = pow(opacity, opacityScale);
	final.a *= opacity;

	// apply over-operator
	#ifdef INVERTFUNCTION
		final = overOperator(scatterPlot, final);
	#else
		final = overOperator(final, scatterPlot);
	#endif
#endif


// Distance based blending 
#ifdef DISTANCEBLENDING

	// convert to range [0,1]:
	// - https://stackoverflow.com/questions/929103/convert-a-number-range-to-another-range-maintaining-ratio
	float oldMin = uintBitsToFloat(minKernelDifference);
	float oldMax = uintBitsToFloat(maxKernelDifference);
	float oldValue = texelFetch(kernelDensityTexture,ivec2(gl_FragCoord.xy),0).g;

	// transform [minKernelDifference, maxKernelDifference] to [newMin, 1]
	float opacity = (oldValue - oldMin) / (oldMax - oldMin);

	// scale opacity and apply to alpha
	opacity = pow(opacity, opacityScale);
	final.a *= opacity;

	scatterPlot.rgb = vec3(1.0);
	scatterPlot.rgb *= (0.5+0.5*light_occlusion*pow(ambient.a,1.0));
	
	// apply over-operator
	#ifdef INVERTFUNCTION
		final = overOperator(scatterPlot, final);
	#else
		final = overOperator(final, scatterPlot);
	#endif
#endif


// Normal based blending
#ifdef NORMALBLENDING

	vec3 centerNormal = texelFetch(surfaceNormalTexture, ivec2(gl_FragCoord.xy), 0).xyz;

	// emphasize the curvature
	float opacity = 1.0f - dot(centerNormal, V);

	// scale opacity and apply to alpha
	opacity = pow(opacity, opacityScale);
	final.a *= opacity;

	// apply over-operator
	#ifdef INVERTFUNCTION
		final = overOperator(scatterPlot, final);
	#else
		final = overOperator(final, scatterPlot);
	#endif
#endif

#ifdef LENSING
	float pxlDistance = length((focusPosition-gFragmentPosition.xy) / vec2(aspectRatio, 1.0));
	
	// anti-aliasing distance -> make lens thickness independent of window size
	float startInner = lensSize - 7.5f /*empirically chosen*/ / windowHeight;
	float endOuter = lensSize + 7.5f /*empirically chosen*/ / windowHeight;

	// draw border of lens
	if(pxlDistance >= startInner && pxlDistance <= lensSize)
	{
		final.rgb = mix(final.rgb, lensBorderColor, smoothstep(startInner, lensSize, pxlDistance));
	}
	else if (pxlDistance > lensSize && pxlDistance <= endOuter)
	{
		final.rgb = mix(final.rgb, lensBorderColor, 1.0f - smoothstep(lensSize, endOuter, pxlDistance));
	}
#endif

	fragColor = final; 

}