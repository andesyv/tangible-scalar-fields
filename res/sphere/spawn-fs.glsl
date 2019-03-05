#version 450

uniform mat4 modelViewProjectionMatrix;
uniform mat4 inverseModelViewProjectionMatrix;

const float PI = 3.14159265359;

// Normal distribution N(mue,sigma^2) 
const float mue = 0.0f;			// mean
uniform float sigma2;			// standard deviation

uniform float gaussScale;

in vec4 gFragmentPosition;
flat in vec4 gSpherePosition;
flat in float gSphereRadius;
flat in float gSphereOriginalRadius;
flat in float gSphereValue;

//out vec4 fragPosition;
//out vec4 fragNormal;
layout (location = 2) out vec4 kernelDensity;

layout(binding = 0) uniform sampler2D positionTexture;
layout(r32ui, binding = 0) uniform uimage2D offsetImage;
//layout(rgba32f, binding = 1) uniform image2D kernelDensity;

struct BufferEntry
{
	float near;
	float far;
	vec3 center;
	float radius;
	float value;
	uint previous;
};

layout(std430, binding = 1) buffer intersectionBuffer
{
	uint count;
	BufferEntry intersections[];
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
float calcDepth(vec3 pos)
{
	float far = gl_DepthRange.far; 
	float near = gl_DepthRange.near;
	vec4 clip_space_pos = modelViewProjectionMatrix * vec4(pos, 1.0);
	float ndc_depth = clip_space_pos.z / clip_space_pos.w;
	return (((far - near) * ndc_depth) + near + far) / 2.0;
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

	vec4 position = texelFetch(positionTexture,ivec2(gl_FragCoord.xy),0);
	BufferEntry entry;
	
	entry.near = length(sphere.near.xyz-near.xyz);
	
	//if (entry.near > position.w)
	//	discard;	

	uint index = atomicAdd(count,1);
	uint prev = imageAtomicExchange(offsetImage,ivec2(gl_FragCoord.xy),index);

	entry.far = length(sphere.far.xyz-near.xyz);

	entry.center = gSpherePosition.xyz;
	entry.radius = gSphereOriginalRadius;
	entry.value = gSphereValue;
	entry.previous = prev;

	intersections[index] = entry;

	// probability density function
	float x = length(gSpherePosition.xy - sphere.near.xy);
	float gaussKernel = 1.0f / (sqrt(2.0f * PI* sigma2)) * exp(-(pow((x-mue),2) / (2 * sigma2)));

	// additional GUI dependent scaling 
	gaussKernel = pow(gaussKernel,gaussScale);

	//imageStore(kernelDensity, ivec2(gl_FragCoord.xy), vec4(gaussKernel, 0.0f, 0.0f, 1.0f));
	kernelDensity = vec4(gaussKernel, 0.0f, 0.0f, 1.0f);
}