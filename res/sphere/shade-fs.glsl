#version 450
#include "/defines.glsl"
#include "/globals.glsl"

layout(pixel_center_integer) in vec4 gl_FragCoord;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 modelViewProjectionMatrix;
uniform mat4 inverseModelViewProjectionMatrix;
uniform mat3 normalMatrix;
uniform mat3 inverseNormalMatrix;

uniform vec3 lightPosition;
uniform vec3 diffuseMaterial;
uniform vec3 ambientMaterial;
uniform vec3 specularMaterial;
uniform float shininess;

uniform sampler2D spherePositionTexture;
uniform sampler2D sphereNormalTexture;
uniform sampler2D sphereDiffuseTexture;
uniform sampler2D surfacePositionTexture;
uniform sampler2D surfaceNormalTexture;
uniform sampler2D surfaceDiffuseTexture;
uniform sampler2D depthTexture;
uniform sampler2D ambientTexture;
uniform sampler2D materialTexture;
uniform sampler2D environmentTexture;
uniform bool environment;

// 1D color map parameters
uniform sampler1D colorMapTexture;
uniform int textureWidth;

uniform float maximumCoCRadius = 0.0;
uniform float aparture = 0.0;
uniform float focalDistance = 0.0;
uniform float focalLength = 0.0;

uniform float distanceBlending = 0.0;
uniform float distanceScale = 1.0;

in vec4 gFragmentPosition;
out vec4 fragColor;

layout(std430, binding = 3) buffer depthRangeBuffer
{
	uint minDepth;
	uint maxDepth;
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

void main()
{
	vec4 fragCoord = gFragmentPosition;
	fragCoord /= fragCoord.w;
	
	vec4 near = inverseModelViewProjectionMatrix*vec4(fragCoord.xy,-1.0,1.0);
	near /= near.w;

	vec4 far = inverseModelViewProjectionMatrix*vec4(fragCoord.xy,1.0,1.0);
	far /= far.w;

	vec3 V = normalize(far.xyz-near.xyz);

	vec4 spherePosition = texelFetch(spherePositionTexture,ivec2(gl_FragCoord.xy),0);
	vec4 sphereNormal = texelFetch(sphereNormalTexture,ivec2(gl_FragCoord.xy),0);
	vec4 sphereDiffuse = texelFetch(sphereDiffuseTexture,ivec2(gl_FragCoord.xy),0);

	vec4 surfacePosition = texelFetch(surfacePositionTexture,ivec2(gl_FragCoord.xy),0);
	vec4 surfaceNormal = texelFetch(surfaceNormalTexture,ivec2(gl_FragCoord.xy),0);
	vec4 surfaceDiffuse = texelFetch(surfaceDiffuseTexture,ivec2(gl_FragCoord.xy),0);
	
	if (surfacePosition.w >= 65535.0)	
		surfaceDiffuse.a = 0.0;

	vec4 backgroundColor = vec4(0.0,0.0,0.0,1.0);

#ifdef ENVIRONMENT
	backgroundColor = textureLod(environmentTexture,latlong(V),0.0);
#endif

	vec4 ambient = vec4(1.0,1.0,1.0,1.0);

#ifdef AMBIENT
	ambient = texelFetch(ambientTexture,ivec2(gl_FragCoord.xy),0);
#endif

	float depth = texelFetch(depthTexture,ivec2(gl_FragCoord.xy),0).x;
	vec3 surfaceNormalWorld = normalize(inverseNormalMatrix*surfaceNormal.xyz);

	vec3 ambientColor = ambientMaterial*ambient.a;
	vec3 diffuseColor = surfaceDiffuse.rgb*diffuseMaterial*ambient.a;
	vec3 specularColor = specularMaterial*ambient.a;

	vec3 N = surfaceNormalWorld;
	vec3 L = normalize(lightPosition-surfacePosition.xyz);
	vec3 R = normalize(reflect(L, N));
	float NdotV = max(0.0,abs(dot(N, V)));
	float NdotL = dot(N, L);
	float RdotV = max(0.0,dot(R, V));
	
	vec4 environmentColor = vec4(1.0,1.0,1.0,1.0);

#ifdef ENVIRONMENT
	float width = sqrt(ambient.r);
	environmentColor = textureGrad(environmentTexture,latlong(N.xyz),vec2(width,0.0),vec2(0.0,width));
#endif

	vec3 color = vec3(0.0);
	float light_occlusion = 1.0;

#ifdef AMBIENT
	vec3 VL =  normalize(normalMatrix*normalize(lightPosition-surfacePosition.xyz));
	light_occlusion = 1.0-clamp(dot(VL.xyz, ambient.xyz),0.0,1.0);
#endif

	float lightRadius = 4.0*length(lightPosition);
	float lightDistance = length(lightPosition.xyz-surfacePosition.xyz) / lightRadius;
	float lightAttenuation = 1.0 / (1.0 + lightDistance*lightDistance);
	light_occlusion *= lightAttenuation;

	NdotL = clamp((NdotL+1.0)*0.5,0.0,1.0);
	color = ambientColor + light_occlusion * (NdotL * diffuseColor + pow(RdotV,shininess) * specularColor);
	color.rgb += distanceBlending*vec3(min(1.0,pow(abs(spherePosition.w-surfacePosition.w),distanceScale)));

	vec4 final = over(vec4(color,1.0)*surfaceDiffuse.a,backgroundColor);

#ifdef DEPTHOFFIELD
	vec4 cp = modelViewMatrix*vec4(surfacePosition.xyz, 1.0);
	cp = cp / cp.w;
	float dist = length(cp.xyz);

	float coc = maximumCoCRadius * aparture * (focalLength * (focalDistance - dist)) / (dist * (focalDistance - focalLength));
	//coc += fwidth(surfaceDiffuse.a);
	coc = clamp( coc * 0.5 + 0.5, 0.0, 1.0 );

	if (surfacePosition.w >= 65535.0)	
		coc = 0.0;

	final.a = coc;
#endif

#ifdef COLORMAP
	if(depth != 1.0f)
	{
	
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

		// apply color map  using texelFetch with a range from [0, size) 
		vec3 colorMap  = texelFetch(colorMapTexture, newMax-int(round(newValue)), 0).rgb; 		

	#ifdef ILLUMINATION
		// MATLAB - rgb2gray: https://www.mathworks.com/help/matlab/ref/rgb2gray.html
		float luminosity = 0.2989f * final.r + 0.5870f * final.g + 0.1140f * final.g;
		final.rgb = colorMap * luminosity;
	#else
		final.rgb = colorMap;
	#endif
	}
#endif

	fragColor = final; 
}