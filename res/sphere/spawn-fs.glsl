#version 450

uniform mat4 modelViewProjection;
uniform mat4 inverseModelViewProjection;
uniform float probeRadius;

in vec4 gFragmentPosition;
in vec4 gSpherePosition;
in float gSphereRadius;
flat in int gVertexId;

out vec4 fragPosition;
out vec4 fragNormal;

precision highp float;

layout(binding = 0) uniform sampler2D positionTexture;
layout(r32ui, binding = 0) uniform uimage2D offsetImage;

struct BufferEntry
{
	vec4 near;
	vec4 far;
	vec3 center;
	float radius;
	uint id;
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
	vec4 clip_space_pos = modelViewProjection * vec4(pos, 1.0);
	float ndc_depth = clip_space_pos.z / clip_space_pos.w;
	return (((far - near) * ndc_depth) + near + far) / 2.0;
}


void main()
{
	vec4 fragCoord = gFragmentPosition;
	fragCoord /= fragCoord.w;
	
	vec4 near = inverseModelViewProjection*vec4(fragCoord.xy,-1.0,1.0);
	near /= near.w;

	vec4 far = inverseModelViewProjection*vec4(fragCoord.xy,1.0,1.0);
	far /= far.w;

	vec3 V = normalize(far.xyz-near.xyz);	
	Sphere sphere = calcSphereIntersection(gSphereRadius*1.0, near.xyz, gSpherePosition.xyz, V);
	
	if (!sphere.hit)
		discard;

	vec4 position = texelFetch(positionTexture,ivec2(gl_FragCoord.xy),0);
	BufferEntry entry;
	
	entry.near.xyz = sphere.near;
	entry.near.w = length(sphere.near.xyz-near.xyz);//calcDepth(sphere.near.xyz);
	
	if (entry.near.w > position.w)
		discard;	

	uint index = atomicAdd(count,1);
	uint prev = imageAtomicExchange(offsetImage,ivec2(gl_FragCoord.xy),index);


	entry.id = gVertexId;
	entry.far.xyz = sphere.far;
	entry.far.w = length(sphere.far.xyz-near.xyz);//calcDepth(sphere.far.xyz);
	/*
	if (entry.far.w < entry.near.w)
	{
		vec4 temp = entry.far;
		entry.far = entry.near;
		entry.near = temp;
	}*/

	entry.center = gSpherePosition.xyz;
	entry.radius = gSphereRadius-probeRadius;
	entry.previous = prev;

	intersections[index] = entry;
	

	//fragPosition = vec4(0.0,0.0,1.0,1.0);//vec4(sphere.far.xyz,1.0);
//	fragPosition = vec4(sphere.near.xyz,1.0);
//	fragNormal = vec4(sphere.normal,1.0);//vec4(normalize(normalMatrix*(mix(vec3(0.0,0.0,-1.0),-N,1.0))),1.0);

//	gl_FragDepth = calcDepth(sphere.near.xyz);
}