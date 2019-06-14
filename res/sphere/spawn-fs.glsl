#version 450
#include "/defines.glsl"

layout(pixel_center_integer) in vec4 gl_FragCoord;

uniform mat4 modelViewMatrix;
uniform mat4 modelViewProjectionMatrix;
uniform mat4 inverseModelViewProjectionMatrix;

uniform float windowWidth;
uniform float windowHeight;

const float PI = 3.14159265359;

// Normal distribution N(mue,sigma^2) 
uniform float sigma2;			// standard deviation
uniform float gaussScale;
uniform float alpha;			// sensitivity parameter

// estimated density from previous render pass
uniform sampler2D pilotDensityTexture;

in vec4 gFragmentPosition;
flat in vec4 gSpherePosition;
flat in float gSphereRadius;
flat in float gSphereValue;

// focus and context
uniform vec2 focusPosition;
uniform float lensSize;
uniform float lensSigma;
uniform float aspectRatio;

// pass number of sample points
uniform float samplesCount;

layout (location = 2) out vec4 kernelDensity;
//layout (location = 3) out vec4 scatterPlott;
//layout (location = 4) out vec4 densityEstimation;

layout(std430, binding = 1) buffer geomMeanBuffer
{
	int geomMean;
};

struct Sphere
{			
	bool hit;
	vec3 near;
	vec3 far;
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

		return Sphere(true, near, far);
	}
	else
	{
		return Sphere(false, vec3(0), vec3(0));
	}
}

float gaussKernel(float x, float sigma)
{
	return 1.0f / (sqrt(2.0f * PI* sigma)) * exp(-(pow((x),2) / (2 * sigma)));
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

// -----------------------------------------------------------------------------------------------------


	float sigmaScale = 1.0f;

#ifdef ADAPTIVEKERNEL
	// scale sigma dependent on camera distance

	// TODO: less magic! --------------------------------------------- 
	//float distanceScaling = pow(entry.near/6500,3);
	vec4 sized = modelViewMatrix * vec4(4024.0f /*distanceScaling*/, 0.0f, 0.0f, 0.0f);
	float radiusd = length(sized);
	// ---------------------------------------------------------------

	sigmaScale = sigma2/pow(radiusd,2);
#endif


	// this sigma is used to calculate height range (prevent flickering if lens is used)
	float originalSigmaScale = sigmaScale;

#ifdef LENSING
	// compute distance to mouse cursor
	float pxlDistance = length((focusPosition-gFragmentPosition.xy)/vec2(aspectRatio,1.0));

	if(pxlDistance <= lensSize)
	{
		// scale sigma down within the lens
		sigmaScale *= lensSigma;
	}
#endif


	float x = length(gSpherePosition.xy - sphere.near.xy);


#ifdef ADAPTIVEKDE
	
	// transform to NDC coordinates including perspective divide
	vec4 sphereCenter = modelViewProjectionMatrix * gSpherePosition;	
	
	// orthographic projection: no change since w-component is still 1.0f
	//sphereCenter /= sphereCenter.w;				

	// transform to viewport coordinates
	sphereCenter.xy = sphereCenter.xy * 0.5f + vec2(0.5f);
	
	// transform to pixel coordinates (only needed for 'texelFetch')
	//sphereCenter.xy *= vec2(windowWidth, windowHeight);

	// Fixpoint arithmetic: convert back to float
	float geomMeanFloat = float(geomMean / 256);

	// 1/n * sum(log(fp)) = log(geometic_mean) --> geometic_mean = exp(log(geometric_mean)
	geomMeanFloat = exp(geomMeanFloat / samplesCount);

	// compute variable bandwidth (lambda)
	//float lambda = pow(texelFetch(pilotDensityTexture,ivec2(sphereCenter.xy), 0).r / geomMeanFloat, -alpha);
	float lambda = pow(texture(pilotDensityTexture, sphereCenter.xy).r / geomMeanFloat, -alpha);

	// apply lambda to distance x
	x = x / lambda;
#endif


	// calculate variable kernel density estimation
	float variableKDE = gaussKernel(x, sigma2*sigmaScale);
	
	// evaluate kernel that is not influenced by the lens interaction
	float originalKernel = gaussKernel(x, sigma2*originalSigmaScale);
	
	// compute difference to current maximum value of the curve
	float difference = gaussKernel(0, sigma2*sigmaScale) - variableKDE;


#ifdef ADAPTIVEKDE	
	originalKernel = originalKernel / lambda;
	variableKDE = variableKDE / lambda;
	difference = difference / lambda;
#endif	


	// additional GUI dependent scaling 
	variableKDE = -pow(variableKDE, gaussScale);
	difference = -pow(difference, gaussScale);
	originalKernel = -pow(originalKernel, gaussScale);


#ifdef LENSING
	if(pxlDistance <= lensSize)
	{
		// scale difference to emphsaize scatter plot within lense
		difference *= sigmaScale;
	}
#endif


	//imageStore(kernelDensity, ivec2(gl_FragCoord.xy), vec4(gaussKernel, difference, 0.0f, 1.0f));
	kernelDensity = vec4(variableKDE, difference, originalKernel, 1.0f);
}