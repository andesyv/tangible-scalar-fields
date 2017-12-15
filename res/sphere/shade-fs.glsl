#version 450

layout(pixel_center_integer) in vec4 gl_FragCoord;

uniform vec3 lightPosition;
uniform vec3 diffuseMaterial;
uniform vec3 ambientMaterial;
uniform vec3 specularMaterial;
uniform float shininess;
uniform mat3 normalMatrix;

uniform sampler2D colorTexture;
uniform sampler2D normalTexture;
uniform sampler2D depthTexture;

out vec4 fragColor;

void main()
{
	vec3 V = vec3(0.0,0.0,1.0);
	vec4 color = texelFetch(colorTexture,ivec2(gl_FragCoord.xy),0);
	vec4 normal = texelFetch(normalTexture,ivec2(gl_FragCoord.xy),0);
	vec4 depth = texelFetch(depthTexture,ivec2(gl_FragCoord.xy),0);

	// vectors
	vec3 N = normalize(normal.xyz);
	vec3 L = lightPosition;
	vec3 R = normalize(reflect(L, N));
	float NdotL = max(0, dot(N, L));
	float RdotV = max(0, dot(R, V));

    vec3 c = ambientMaterial*color.rgb + NdotL * diffuseMaterial + pow(RdotV,shininess) * specularMaterial;
	//vec3 c = color;// color.rgb*vec3(63.0/255.0,136/255.0,189/255.0);// + 0.125*color.rgb*pow(RdotV,shininess) * specularMaterial;//0.5*color.rgb + 0.5*color.rgb*(0.75+0.25 * NdotL) * diffuseMaterial + color.rgb*pow(RdotV,shininess) * specularMaterial;

	fragColor = vec4(c,1.0);
	gl_FragDepth = depth.z;
}