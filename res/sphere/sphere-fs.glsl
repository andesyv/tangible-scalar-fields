#version 450

uniform mat4 projection;

uniform vec3 lightPosition;
uniform vec3 diffuseMaterial;
uniform vec3 ambientMaterial;
uniform vec3 specularMaterial;
uniform float shininess;
uniform mat3 normalMatrix;

in vec4 gSpherePosition;
in vec4 gFragmentPosition;
in float gSphereRadius;

out vec4 fragColor;
out vec4 fragNormal;

precision highp float;

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
		vec3 near = origin + min(da, ds) * l;
		vec3 far = origin + max(da, ds) * l;
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
	vec4 clip_space_pos = projection * vec4(pos, 1.0);
	float ndc_depth = clip_space_pos.z / clip_space_pos.w;
	return (((far - near) * ndc_depth) + near + far) / 2.0;
}


void main()
{
	vec3 V = normalize(gFragmentPosition.xyz);
	
	Sphere sphere = calcSphereIntersection(gSphereRadius, vec3(0.0), gSpherePosition.xyz, V);
	
	if (!sphere.hit)
		discard;

	// vectors
	vec3 N = sphere.normal;
	vec3 L = normalize(lightPosition - sphere.near);
	vec3 R = normalize(reflect(L, N));
	float NdotL = max(0, dot(N, L));
	float RdotV = max(0, dot(R, V));

    //vec3 color = ambientMaterial*16.0;// + NdotL * diffuseMaterial + pow(RdotV,shininess) * specularMaterial;
	vec3 color = vec3(1.0,1.0,1.0);//+pow(RdotV,shininess) * specularMaterial;//ambientMaterial + NdotL * diffuseMaterial + pow(RdotV,shininess) * specularMaterial;
	/*
	float NdotV = dot(N,-V);

	float d = fwidth(NdotV);
    float s = smoothstep(0.0, mix(1.0,10.0*d,0.125), 2.0*NdotV);
	float a = smoothstep(0.0, d*3.0, NdotV);
	*/
	fragColor = vec4(color,1.0);
	fragNormal = vec4(-sphere.normal,1.0);//vec4(normalize(normalMatrix*(mix(vec3(0.0,0.0,-1.0),-N,1.0))),1.0);
	gl_FragDepth = calcDepth(sphere.near.xyz);
}