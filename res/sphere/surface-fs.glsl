#version 450
#extension GL_ARB_shading_language_include : require
#include "/defines.glsl"
#include "/globals.glsl"

layout(pixel_center_integer) in vec4 gl_FragCoord;

uniform mat4 modelViewMatrix;
uniform mat4 modelViewProjectionMatrix;
uniform mat4 inverseModelViewProjectionMatrix;

uniform sampler2D kernelDensityTexture;
uniform sampler2D scatterPlotTexture;

in vec4 gFragmentPosition;
out vec4 surfacePosition;
out vec4 surfaceNormal;
out vec4 surfaceDiffuse;
out vec4 sphereDiffuse;

layout(std430, binding = 3) buffer depthRangeBuffer
{
	uint minDepth;
	uint maxDepth;
};

float calcDepth(vec3 pos)
{
	float far = gl_DepthRange.far; 
	float near = gl_DepthRange.near;
	vec4 clip_space_pos = modelViewProjectionMatrix * vec4(pos, 1.0);
	float ndc_depth = clip_space_pos.z / clip_space_pos.w;
	return (((far - near) * ndc_depth) + near + far) / 2.0;
}

// https://stackoverflow.com/questions/49640250/calculate-normals-from-heightmap
vec3 calculateNormalFromHeightMap(ivec2 texelCoord){
	
	// texelFetch (0,0) -> left bottom
	float top = texelFetch(kernelDensityTexture, ivec2(texelCoord.x, texelCoord.y+1), 0).r;
	float bottom = texelFetch(kernelDensityTexture, ivec2(texelCoord.x, texelCoord.y-1), 0).r;
	float left = texelFetch(kernelDensityTexture, ivec2(texelCoord.x-1, texelCoord.y), 0).r;
	float right = texelFetch(kernelDensityTexture, ivec2(texelCoord.x+1, texelCoord.y), 0).r;

	// reconstruct normal
	vec3 normal = vec3((right-left)/2,(top-bottom)/2,1);
	return normalize(normal);
}

void main()
{

	// apply density estimation
	float kernelDensity = texelFetch(kernelDensityTexture,ivec2(gl_FragCoord.xy),0).r;

	if(kernelDensity == 0.0f)
		discard;

	surfacePosition = vec4(gl_FragCoord.xy, kernelDensity, 1);
	surfaceNormal = vec4(calculateNormalFromHeightMap(ivec2(gl_FragCoord.xy)), 1);

	// set diffuse colors
	surfaceDiffuse = vec4(1.0f, 1.0f, 1.0f, 1.0f); 
	sphereDiffuse = vec4(1.0f, 1.0f, 1.0f, 1.0f); 

	gl_FragDepth = calcDepth(surfacePosition.xyz);

	if(gl_FragDepth < 1.0f)
	{
		
		vec4 fragCoord = gFragmentPosition;
		fragCoord /= fragCoord.w;

		vec4 near = inverseModelViewProjectionMatrix*vec4(fragCoord.xy,-1.0,1.0);
		near /= near.w;

		vec4 far = inverseModelViewProjectionMatrix*vec4(fragCoord.xy,1.0,1.0);
		far /= far.w;

		vec3 V = normalize(far.xyz-near.xyz);

		// compute position in view-space and calculate normal distance (using the camera view-vector)
		vec4 posViewSpace = modelViewMatrix*vec4(gl_FragCoord.xy, texelFetch(kernelDensityTexture,ivec2(gl_FragCoord.xy),0).b, 1);
		float cameraDistance = abs(dot(posViewSpace.xyz,V));

		// floatBitsToUint: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/floatBitsToInt.xhtml
		uint uintDepth = floatBitsToUint(cameraDistance);
		
		// identify depth-range
		atomicMin(minDepth, uintDepth);
		atomicMax(maxDepth, uintDepth);
	}
}