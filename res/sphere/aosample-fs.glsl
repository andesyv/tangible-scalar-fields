#version 450

// total number of samples at each fragment
#define PI						3.1415926535897932384626433832795
#define NUM_SAMPLES				32
#define NUM_SPIRAL_TURNS		7
#define VARIATION				1

layout(pixel_center_integer) in vec4 gl_FragCoord;
in vec4 gFragmentPosition;
out vec4 fragAmbient;

uniform sampler2D surfaceNormalTexture;

uniform vec4 projectionInfo;
uniform float projectionScale;

uniform float occlusionRadius = 1.0;
uniform float occlusionBias = 0.0;//0.012;
uniform float occlusionIntensity = 1.0;

const float occlusionRadius2 = occlusionRadius * occlusionRadius;

// returns a unit vector and a screen-space radius for the tap on a unit disk 
// (the caller should scale by the actual disk radius)
vec2 tapLocation(int sampleNumber, float spinAngle, out float radiusSS)
{
	// radius relative to radiusSS
	float alpha = (float(sampleNumber) + 0.5) * (1.0 / float(NUM_SAMPLES));
	float angle = alpha * (float(NUM_SPIRAL_TURNS) * 6.28) + spinAngle;

	radiusSS = alpha;
	return vec2(cos(angle), sin(angle));
}

vec3 getPosition(ivec2 positionSS)
{
	float depth = texelFetch(surfaceNormalTexture,positionSS,0).a;	
	return vec3((positionSS * projectionInfo.xy + projectionInfo.zw) * depth, depth);
}

vec3 getOffsetPosition(ivec2 positionSS, vec2 unitOffset, float radiusSS)
{
	return getPosition(positionSS + ivec2(radiusSS * unitOffset));
}

vec4 sampleAO(ivec2 positionSS, vec3 positionVS, vec3 normalVS, float sampleRadiusSS,  int tapIndex, float rotationAngle)
{
	const float epsilon = 0.01;

	// offset on the unit disk, spun for this pixel
	float radiusSS;

	vec2 unitOffset = tapLocation(tapIndex, rotationAngle, radiusSS);
	radiusSS *= sampleRadiusSS;

	vec3 Q = getOffsetPosition(positionSS, unitOffset, radiusSS);
	vec3 v = Q - positionVS;
	
	float vv = dot(v, v);
	float vn = dot(v, normalVS) - occlusionBias;

#if VARIATION == 0  

	// (from the HPG12 paper)
	// Note large epsilon to avoid overdarkening within cracks
	return float(vv < occlusionRadius2) * max(vn / (epsilon + vv), 0.0);

#elif VARIATION == 1 // default / recommended
  
	// Smoother transition to zero (lowers contrast, smoothing out corners). [Recommended]
	//float f = max(occlusionRadius2 - vv, 0.0) / occlusionRadius2;
	//float weight = f * f * f * max(vn / (epsilon + vv), 0.0);

	float dist = length(v);
	vec3 vnorm = v / dist;

	float f = max(occlusionRadius2 - vv, 0.0) / occlusionRadius2;

	vec4 occlusion = vec4(0.0);
	occlusion.w = f * f * f * max(vn / (epsilon + vv), 0.0);
	occlusion.xyz = 2.0*vnorm*occlusion.w;
	/*
	float attenuation = 1.0-clamp(dist / occlusionRadius,0.0,1.0);
	attenuation = attenuation*attenuation * step(0.0, vn/dist);
	occlusion.xyz = attenuation * 5.0 * sqrt(3.0/PI) * vnorm;
	*/
	return occlusion;
	//return vec4(normalize(v),1.0) * weight;

#elif VARIATION == 2
  
	// Medium contrast (which looks better at high radii), no division.  Note that the 
	// contribution still falls off with radius^2, but we've adjusted the rate in a way that is
	// more computationally efficient and happens to be aesthetically pleasing.
	return 4.0 * max(1.0 - vv / occlusionRadius2, 0.0) * max(vn, 0.0);

#else 

	// Low contrast, no division operation
	return 2.0 * float(vv < occlusionRadius2) * max(vn, 0.0);

#endif
}

float rand(vec2 co)
{
	float a = 12.9898;
    float b = 78.233;
    float c = 43758.5453;
    float dt= dot(co.xy ,vec2(a,b));
    float sn= mod(dt,3.14);
	return fract(sin(sn) * c);
}


void main()
{
	ivec2 positionSS = ivec2(gl_FragCoord.xy);

	vec3 positionVS = getPosition(positionSS);
	vec3 normalVS = texelFetch(surfaceNormalTexture,positionSS,0).xyz;
  
	float sampleNoise = rand(positionSS);
	float randomPatternRotationAngle = 2.0 * PI * sampleNoise;
	//float randomPatternRotationAngle = (3 * positionSS.x ^ positionSS.y + positionSS.x * positionSS.y) * 10;


	vec4 occlusion = vec4(0.0);
	float radiusSS = -(projectionScale * occlusionRadius) / positionVS.z;
	
	for (int i = 0; i < NUM_SAMPLES; ++i)
	{
		occlusion += sampleAO(positionSS, positionVS, normalVS, radiusSS, i, randomPatternRotationAngle);
	}

	occlusion /= float(NUM_SAMPLES);
	occlusion.w = clamp(pow(1.0-occlusion.w, 1.0 + occlusionIntensity), 0.0, 1.0);
  
	//occlusion = max(0.0, 1.0 - occlusion * (occlusionIntensity/pow(occlusionRadius,6.0)) * (5.0 / NUM_SAMPLES));
	//occlusion = 1.0 - occlusion / (float(NUM_SAMPLES));
	//occlusion = 1.0 - occlusion / (4.0 * float(NUM_SAMPLES));
	//occlusion = clamp(pow(occlusion, 1.0 + occlusionIntensity), 0.0, 1.0);
	//occlusion /= NUM_SAMPLES;

    // Bilateral box-filter over a quad for free, respecting depth edges
    // (the difference that this makes is subtle)
	/*
    if (abs(dFdx(positionVS.z)) < 0.02)
	{
        occlusion -= dFdx(occlusion) * ((positionSS.x & 1) - 0.5);
    }
    if (abs(dFdy(positionVS.z)) < 0.02)
	{
        occlusion -= dFdy(occlusion) * ((positionSS.y & 1) - 0.5);
    }
	*/
	fragAmbient = occlusion;//vec4(occlusion, occlusion, occlusion, positionVS.z);
}
