#version 450

layout(pixel_center_integer) in vec4 gl_FragCoord;

uniform mat4 inverseModelViewProjection;
uniform sampler2D colorTexture;
uniform sampler2D normalTexture;
uniform sampler2D depthTexture;
uniform sampler2D environmentTexture;
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

void main()
{
	vec4 color = texelFetch(colorTexture,ivec2(gl_FragCoord.xy),0);
	vec3 normal = texelFetch(normalTexture,ivec2(gl_FragCoord.xy),0).xyz;
	float depth = texelFetch(depthTexture,ivec2(gl_FragCoord.xy),0).x;

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
	}

	fragColor = color;
	gl_FragDepth = depth;
}