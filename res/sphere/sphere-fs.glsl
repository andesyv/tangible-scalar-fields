#version 450
#include "/defines.glsl"

uniform mat4 modelViewProjectionMatrix;
uniform mat4 inverseModelViewProjectionMatrix;
//uniform float smallestR;

in vec4 gFragmentPosition;
flat in vec4 gSpherePosition;
flat in float gSphereRadius;
//flat in float gSphereOriginalRadius;
flat in float gSphereValue;

//layout (location = 2) out vec4 kernelDensity;
layout (location = 3) out vec4 scatterPlot;
//layout (location = 4) out vec4 densityEstimation;

struct Sphere
{			
	bool hit;
	vec3 near;
	vec3 far;
	vec3 normal;
};
																					
Sphere calcSphereIntersection(float r, vec3 origin, vec3 center, vec3 line)
{
	vec3 oc = origin - center;
	vec3 l = normalize(line);
	float loc = dot(l, oc);
	float under_square_root = loc * loc - dot(oc, oc) + r*r;
	if (under_square_root > 0)
	{
		float da = -loc + sqrt(under_square_root);
		float ds = -loc - sqrt(under_square_root);
		vec3 near = origin+min(da, ds) * l;
		vec3 far = origin+max(da, ds) * l;
		vec3 normal = (near - center);

		return Sphere(true, near, far, normal);
	}
	else
	{
		return Sphere(false, vec3(0), vec3(0), vec3(0));
	}
}

float calcDepth(vec3 pos)
{
	float far = gl_DepthRange.far; 
	float near = gl_DepthRange.near;
	vec4 clip_space_pos = modelViewProjectionMatrix * vec4(pos, 1.0);
	float ndc_depth = clip_space_pos.z / clip_space_pos.w;
	return (((far - near) * ndc_depth) + near + far) / 2.0;
}

// From: http://colorbrewer2.org/#type=qualitative&scheme=Paired&n=12
// input: index between (0-11) to access color-map for qualitative data
vec4 assignQualitativeColor (int index)
{
	
	// select red as default color
	vec4 sphereColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);

	// return red if index is not within the dimension [0,11]
	if(index < 0 || index > 11)
	{
		return sphereColor;
	}

#ifdef COLORSCHEME1
	vec4 quantitativeColors[12] = vec4[12](
		vec4(0.650f, 0.807f, 0.890f, 1.0f),
		vec4(0.121f, 0.470f, 0.705f, 1.0f),
		vec4(0.698f, 0.874f, 0.541f, 1.0f),
		vec4(0.200f, 0.627f, 0.172f, 1.0f),
		vec4(0.984f, 0.603f, 0.600f, 1.0f),
		vec4(0.890f, 0.101f, 0.109f, 1.0f),
		vec4(0.992f, 0.749f, 0.435f, 1.0f),
		vec4(1.000f, 0.498f, 0.000f, 1.0f),
		vec4(0.792f, 0.698f, 0.839f, 1.0f),
		vec4(0.415f, 0.239f, 0.603f, 1.0f),
		vec4(1.000f, 1.000f, 0.600f, 1.0f),
		vec4(0.694f, 0.349f, 0.156f, 1.0f) 	
	);

	sphereColor = quantitativeColors[index];
#endif

#ifdef COLORSCHEME2
	vec4 quantitativeColors[12] = vec4[12](
		vec4(0.552f, 0.827f, 0.780f, 1.0f),
		vec4(1.000f, 1.000f, 0.701f, 1.0f),
		vec4(0.745f, 0.729f, 0.854f, 1.0f),
		vec4(0.984f, 0.501f, 0.447f, 1.0f),
		vec4(0.501f, 0.694f, 0.827f, 1.0f),
		vec4(0.992f, 0.705f, 0.384f, 1.0f),
		vec4(0.701f, 0.870f, 0.411f, 1.0f),
		vec4(0.988f, 0.803f, 0.898f, 1.0f),
		vec4(0.850f, 0.850f, 0.850f, 1.0f),
		vec4(0.737f, 0.501f, 0.741f, 1.0f),
		vec4(0.800f, 1.921f, 0.772f, 1.0f),
		vec4(1.000f, 0.929f, 0.435f, 1.0f)	
	);

	sphereColor = quantitativeColors[index];
#endif

	return sphereColor;
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
	Sphere sphere = calcSphereIntersection(gSphereRadius, near.xyz, gSpherePosition.xyz, V);
	
	if (!sphere.hit)
		discard;

	//fragPosition = vec4(sphere.near.xyz,length(sphere.near.xyz-near.xyz));
	//fragNormal = vec4(sphere.normal, 1.0f);

	float depth = calcDepth(sphere.near.xyz);
	gl_FragDepth = depth;

	// --------------------------------------------------------------------------------------------------------------------

	// Stefan-03-15-2019 -----------------------------------------------------------------------------------------
	// perform Hermite interpolation: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/smoothstep.xhtml
	float factor= 1.0-smoothstep(gSphereRadius*0.75,gSphereRadius,length(sphere.near.xy-gSpherePosition.xy));
	factor += 1.0-smoothstep(gSphereRadius*0.5,gSphereRadius*0.75,length(sphere.near.xy-gSpherePosition.xy));
	scatterPlot = 0.25*factor*assignQualitativeColor(int(gSphereValue));
	// -----------------------------------------------------------------------------------------------------------

	// create classical 2D scatter plot and assign colors according to a qualitative coloring
	//scatterPlot = assignQualitativeColor(int(gSphereValue));
	//
	//float borderDistance = length(fragPosition.xy - gSpherePosition.xy);
	//
	//float firstBorderStart = gSphereRadius - (smallestR * (1.0f - 0.7f /*percentage*/));
	//float secondBorderStart = gSphereRadius - (smallestR * (1.0f - 0.855f /*percentage*/));
	//float thirdBorderStart = gSphereRadius - (smallestR  * (1.0f - 0.86f /*percentage*/));
	//
	//
	//// highlight sphere-border and perform antialiasing within scatterplot
	//if(borderDistance >= firstBorderStart)
	//{
	//
	//	// fade original sphere color to border color 
	//	float fadeOut = (borderDistance-firstBorderStart) / (secondBorderStart-firstBorderStart);
	//
	//	// Smoothstep: https://en.wikipedia.org/wiki/Smoothstep
	//	float smoothFade = 3.0f * pow(fadeOut, 2.0f) - 2.0f  * pow(fadeOut, 3.0f);
	//	scatterPlot.rgb += vec3(smoothFade, smoothFade, smoothFade);
	//
	//	if(borderDistance >= secondBorderStart)
	//	{
	//
	//		// draw white border
	//		scatterPlot.rgb = vec3(1.0f, 1.0f, 1.0f);
	//
	//		if(borderDistance > thirdBorderStart)
	//		{
	//			// perform antialiasing on border smooth-out
	//			fadeOut = 1.0f -(borderDistance-thirdBorderStart) / (gSphereRadius-thirdBorderStart);
	//		
	//			// Smoothstep: https://en.wikipedia.org/wiki/Smoothstep
	//			smoothFade = fadeOut;//3.0f * pow(fadeOut, 2.0f) - 2.0f  * pow(fadeOut, 3.0f);
	//			scatterPlot.rgb = vec3(smoothFade, smoothFade, smoothFade);
	//		}
	//	}
	//}
}