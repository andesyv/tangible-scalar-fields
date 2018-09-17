#version 450

layout(pixel_center_integer) in vec4 gl_FragCoord;

uniform mat4 inverseModelViewProjection;

uniform sampler2D spherePositionTexture;
uniform sampler2D sphereNormalTexture;
uniform sampler2D sphereDiffuseTexture;
uniform sampler2D surfacePositionTexture;
uniform sampler2D surfaceNormalTexture;
uniform sampler2D surfaceDiffuseTexture;
uniform sampler2D ambientTexture;
uniform sampler2D environmentTexture;
uniform sampler2D depthTexture;
uniform bool environment;

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
	vec4 spherePosition = texelFetch(spherePositionTexture,ivec2(gl_FragCoord.xy),0);
	vec4 sphereNormal = texelFetch(sphereNormalTexture,ivec2(gl_FragCoord.xy),0);
	vec4 sphereDiffuse = texelFetch(sphereDiffuseTexture,ivec2(gl_FragCoord.xy),0);

	vec4 surfacePosition = texelFetch(surfacePositionTexture,ivec2(gl_FragCoord.xy),0);
	vec4 surfaceNormal = texelFetch(surfaceNormalTexture,ivec2(gl_FragCoord.xy),0);
	vec4 surfaceDiffuse = texelFetch(surfaceDiffuseTexture,ivec2(gl_FragCoord.xy),0);

	vec4 ambient = texelFetch(ambientTexture,ivec2(gl_FragCoord.xy),0);
	float depth = texelFetch(depthTexture,ivec2(gl_FragCoord.xy),0).x;
	/*
	if (environment)
	{
		vec4 fragCoord = gFragmentPosition;
		fragCoord /= fragCoord.w;
	
		vec4 near = inverseModelViewProjection*vec4(fragCoord.xy,-1.0,1.0);
		near /= near.w;

		vec4 far = inverseModelViewProjection*vec4(fragCoord.xy,1.0,1.0);
		far /= far.w;

		vec3 V = normalize(far.xyz-near.xyz);

		vec2 uv = latlong(V);
		vec4 environmentColor = texture(environmentTexture,uv);
		environmentColor *= (1.0-color.a);
		color += environmentColor;
	}*/

	vec4 surfaceColor = surfaceDiffuse;
	
	if (spherePosition.w < 65535.0 && surfacePosition.w < 65535.0)
		surfaceColor.rgb += 0.5*vec3(min(1.0,0.75+0.25*abs(spherePosition.w-surfacePosition.w)));
	
	surfaceColor.rgb *= ambient.rrr;

	surfaceColor.rgb *= surfaceColor.a;

	vec4 finalColor;
	finalColor = surfaceColor;//over(surfaceColor,sphereDiffuse);


	fragColor = finalColor;
	gl_FragDepth = depth;
}