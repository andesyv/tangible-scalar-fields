#version 450
#include "/defines.glsl"

uniform mat4 modelViewMatrix;
uniform mat4 modelViewProjectionMatrix;
uniform mat4 inverseModelViewProjectionMatrix;

const float PI = 3.14159265359;

// Normal distribution N(mue,sigma^2) 
uniform float sigma2;			
uniform float gaussScale;

in vec4 gFragmentPosition;
flat in vec4 gSpherePosition;
flat in float gSphereRadius;
flat in float gSphereValue;

//layout (location = 2) out vec4 kernelDensity;
//layout (location = 3) out vec4 scatterPlott;
layout (location = 4) out vec4 densityEstimation;

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


	// Pilot Density Estimation ----------------------------------------------------------------------------
	
	float x = length(gSpherePosition.xy - sphere.near.xy);
	float pilotDensity = gaussKernel(x, sigma2);
	
	// local approximation of the geometric mean:---------------------------
	// compute geometric mean --> log(geometic_mean) = 1/n * sum(log(fp))
	//float fp = log(pilotDensity);
	//----------------------------------------------------------------------

	densityEstimation = vec4(pilotDensity, /*fp*/ 0.0f, 0.0f, /*n*/ 1.0f);
	// -----------------------------------------------------------------------------------------------------
}