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

	vec3 ambientColor = ambientMaterial*ambient.rgb;
	vec3 diffuseColor = surfaceDiffuse.rgb*diffuseMaterial*ambient.rgb;
	vec3 specularColor = specularMaterial*ambient.rgb;

	vec3 N = surfaceNormalWorld;
	vec3 L = normalize(lightPosition-surfacePosition.xyz);
	vec3 R = normalize(reflect(L, N));
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
	/*
	float useSSS = 0.5;
	float rim = 1.75 * max( 0., abs( dot( normalize( surfaceNormal.xyz ), normalize( -vOPosition.xyz ) ) ) );
	base += useSSS * vec3(0.2) * ( 1. - .75 * rim );
	base += ( 1. - useSSS ) * 10. * base * vec3(0.2) * clamp( 1. - rim, 0., .15 );
	*/
	//base = vec3( 1. ) - ( vec3( 1. ) - base ) * ( vec3( 1. ) - base );
	vec3 color = ambientColor + NdotL * diffuseColor + pow(RdotV,shininess) * specularColor;
	
	//if (spherePosition.w < 65535.0 && surfacePosition.w < 65535.0)
		color.rgb += 0.25*vec3(min(1.0,0.5+0.5*abs(surfacePosition.w-spherePosition.w))).r;
	
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
	vec4 final = over(vec4(color,1.0)*surfaceDiffuse.a,backgroundColor);

	vec4 cp = modelViewMatrix*vec4(surfacePosition.xyz, 1.0);
	cp = cp / cp.w;
	float dist = length(cp);

	float coc = maximumCoCRadius * aparture * (focalLength * (focalDistance - dist)) / (dist * (focalDistance - focalLength));
	coc = clamp( coc * 0.5 + 0.5, 0.0, 1.0 );

	if (surfacePosition.w >= 65535.0)	
		coc = 0.0;


	final.a = coc;
	fragColor = final; 


	//gl_FragDepth = depth;
}