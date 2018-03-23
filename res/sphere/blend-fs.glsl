#version 450

layout(pixel_center_integer) in vec4 gl_FragCoord;

uniform sampler2D colorTexture;
uniform sampler2D normalTexture;
uniform sampler2D depthTexture;

out vec4 fragColor;

void main()
{
	vec4 color = texelFetch(colorTexture,ivec2(gl_FragCoord.xy),0);
	vec3 normal = texelFetch(normalTexture,ivec2(gl_FragCoord.xy),0).xyz;
	float depth = texelFetch(depthTexture,ivec2(gl_FragCoord.xy),0).x;
	fragColor = color;
	gl_FragDepth = depth;
}