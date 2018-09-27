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

uniform float maximumCoCRadius = 0.0;
uniform float aparture = 0.0;
uniform float focalDistance = 0.0;
uniform float focalLength = 0.0;

uniform float distanceBlending = 0.0;
uniform float distanceScale = 1.0;

in vec4 gFragmentPosition;
out vec4 fragColor;

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

//	surfaceDiffuse.a= aastep(0.0,surfaceDiffuse.a);//vec3(fw,0.0,0.0);


	vec4 backgroundColor = vec4(0.0,0.0,0.0,1.0);

#ifdef ENVIRONMENT
	backgroundColor = textureLod(environmentTexture,latlong(V),0.0);
#endif

	vec4 ambient = vec4(1.0,1.0,1.0,1.0);

#ifdef AMBIENT
	ambient = texelFetch(ambientTexture,ivec2(gl_FragCoord.xy),0);
#endif

	float depth = texelFetch(depthTexture,ivec2(gl_FragCoord.xy),0).x;


	//ambient.xyz = (inverseNormalMatrix*ambient.xyz);

	vec3 surfaceNormalWorld = normalize(inverseNormalMatrix*surfaceNormal.xyz);

	vec3 ambientColor = ambientMaterial*ambient.a;
	vec3 diffuseColor = surfaceDiffuse.rgb*diffuseMaterial*ambient.a;
	vec3 specularColor = specularMaterial*ambient.a;

	vec3 N = surfaceNormalWorld;
	vec3 L = normalize(lightPosition-surfacePosition.xyz);
	vec3 R = normalize(reflect(L, N));
	float NdotV = max(0.0,abs(dot(N, V)));
	float NdotL = max(0.0,dot(N, L));
	float RdotV = max(0.0,dot(R, V));
	
	vec4 environmentColor = vec4(1.0,1.0,1.0,1.0);

#ifdef ENVIRONMENT
	float width = sqrt(ambient.r);
	environmentColor = textureGrad(environmentTexture,latlong(N.xyz),vec2(width,0.0),vec2(0.0,width));
#endif
/*
	vec3 vOPosition = vec3(modelViewMatrix * vec4( surfacePosition.xyz, 1.0 ));
    vec3 vU = normalize( vec3( modelViewMatrix * vec4( surfacePosition.xyz, 1.0 ) ) );

    vec3 r = reflect( normalize( vU ), normalize( surfaceNormal.xyz ) );
    float m = 2.0 * sqrt( r.x * r.x + r.y * r.y + ( r.z + 1.0 ) * ( r.z + 1.0 ) );
    vec2 calculatedNormal = vec2( r.x / m + 0.5,  r.y / m + 0.5 );

    vec3 base = texture2D( materialTexture , calculatedNormal ).rgb;
	base = vec3( 1. ) - ( vec3( 1. ) - base ) * ( vec3( 1. ) - base );
*/
	
	
	//base = vec3( 1. ) - ( vec3( 1. ) - base ) * ( vec3( 1. ) - base );
	//vec3 color = ambientColor + NdotL * diffuseColor + pow(RdotV,shininess) * specularColor;
	vec3 color = vec3(0.0);
#ifdef AMBIENT
	vec3 VL =  normalize(normalMatrix*normalize(lightPosition-surfacePosition.xyz));
	float light_occlusion = 1.0-clamp(dot(VL.xyz, ambient.xyz),0.0,1.0);
#else
	float light_occlusion = 1.0;
#endif
	//light_occlusion = 1.0;
	float lightRadius = 2.0*length(lightPosition);
	float lightDistance = length(lightPosition.xyz-surfacePosition.xyz) / lightRadius;
	float lightAttenuation = 1.0 / (1.0 + lightDistance*lightDistance);
	light_occlusion *= lightAttenuation;


	ivec2 environmentSize = textureSize(environmentTexture,0);
	vec4 environmentColorDiff = textureLod(environmentTexture,latlong(N),textureQueryLevels(environmentTexture));

	float mipLevel = log2(length(environmentSize.xy)) - 0.5 * log2(shininess + 1.0);
	vec4 environmentColorSpec = textureLod(environmentTexture,latlong(R),mipLevel);

//	width = ambient.
//	vec4 environmentColorAmb = textureGrad(environmentTexture,latlong(N),vec2(width,0.0),vec2(0.0,width));

	//color.rgb += (vec3(light_occlusion)*NdotL)*vec3(ambient.w);//light_occlusion;//*NdotL;
	//color = vec3(light_occlusion);//ambientColor + light_occlusion * NdotL * diffuseColor + light_occlusion * pow(RdotV,shininess) * specularColor;
	//color = environmentColor.rgb;//ambientColor + light_occlusion * NdotL * diffuseColor + light_occlusion * pow(RdotV,shininess) * specularColor;

	color = ambientColor + light_occlusion * (NdotL * diffuseColor + pow(RdotV,shininess) * specularColor);
	//color.rgb += vec3(0.125f * pow(1.0-min(ambient.w,1.0),2.0));

	//color.rgb += 0.25*vec3(min(1.0,pow(abs(spherePosition.w-surfacePosition.w),2.0)));
	//color.rgb += 0.25*vec3(min(1.0,0.5+0.25*abs(spherePosition.w-surfacePosition.w)));
	/*
	float useSSS = 0.5;
	float rim = 1.75 * NdotV;
	color += useSSS * vec3(0.2) * ( 1. - .75 * rim );
	color += ( 1. - useSSS ) * 10. * color * vec3(0.2) * clamp( 1. - rim, 0., .15 );
	*/
	//if (spherePosition.w < 65535.0 && surfacePosition.w < 65535.0)
	//color.rgb += 0.25*vec3(min(1.0,pow(abs(spherePosition.w-surfacePosition.w),1.0)));
	//color.rgb += 1.0*vec3(min(1.0,smoothstep(0.0,8.0,abs(spherePosition.w-surfacePosition.w))));
	color.rgb = mix(color.rgb,vec3(smoothstep(0.0,distanceScale,abs(spherePosition.w-surfacePosition.w))),distanceBlending);
	// *(1.0-pow(NdotV,1.5))
	/*
	if (spherePosition.w < 65535.0 && surfacePosition.w < 65535.0)
		color.rgb += 0.25*vec3(min(1.0,0.5+0.25*abs(spherePosition.w-surfacePosition.w)));
	*/
	/*
	vec4 surfaceColor = surfaceDiffuse;
	
	if (spherePosition.w < 65535.0 && surfacePosition.w < 65535.0)
		color.rgb += 0.5*vec3(min(1.0,0.75+0.25*abs(spherePosition.w-surfacePosition.w)));
	
	surfaceColor.rgb *= ambient.rrr;


	surfaceColor.rgb *= surfaceColor.a;
	*/

	//color *= ambient.rrr;
	//vec3 gamma = vec3(1.0/2.2);
    //color = vec3(pow(color, gamma));

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


	fragColor = final; 


	//gl_FragDepth = depth;
}