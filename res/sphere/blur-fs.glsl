#version 450

layout(pixel_center_integer) in vec4 gl_FragCoord;

uniform mat4 inverseModelViewProjection;
uniform sampler2D colorTexture;
uniform sampler2D normalTexture;
uniform sampler2D depthTexture;
uniform sampler2D environmentTexture;
uniform bool environment;

//new
uniform int uMaxCoCRadiusPixels;
uniform int uNearBlurRadiusPixels;
uniform float uInvNearBlurRadiusPixels;

uniform bool horizontal;
//new


in vec4 gFragmentPosition;
out vec4 fragColor;


bool inNearField(in float radiusPixels) {
	return radiusPixels > 0.25;
}

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

	ivec2 kDirection = ivec2(0, 1);

	if (horizontal)
	 ivec2 kDirection = ivec2(1, 0);


	const int KERNEL_TAPS = 6;
	float kernel[KERNEL_TAPS + 1];

	// 11 x 11 separated kernel weights.  This does not dictate the 
	// blur kernel for depth of field; it is scaled to the actual
	// kernel at each pixel.
	//   Custom disk-like // vs. Gaussian
	kernel[6] = 0.00;     // 0.00000000000000;  // Weight applied to outside-radius values
	kernel[5] = 0.50;     // 0.04153263993208;
	kernel[4] = 0.60;     // 0.06352050813141;
	kernel[3] = 0.75;     // 0.08822292796029;
	kernel[2] = 0.90;     // 0.11143948794984;
	kernel[1] = 1.00;     // 0.12815541114232;
	kernel[0] = 1.00; // 0.13425804976814;

	vec4 blurResult = vec4(0.0); //vec3!!
	float blurWeightSum = 0.0;

	vec4 nearResult = vec4(0.0);
	float nearWeightSum = 0.0;

	// Location of the central filter tap (i.e., "this" pixel's location)
	// Account for the scaling down to 25% of original dimensions during blur
	ivec2 A = ivec2(gl_FragCoord.xy * (kDirection * 3 + ivec2(1)));

	float packedA = texelFetch(colorTexture, A, 0).a;
	float r_A = (packedA * 2.0 - 1.0) * uMaxCoCRadiusPixels;

	// Map r_A << 0 to 0, r_A >> 0 to 1
	float nearFieldness_A = clamp(r_A * 4.0, 0.0, 1.0);

	

	for (int delta = -uMaxCoCRadiusPixels; delta <= uMaxCoCRadiusPixels; ++delta) //
	{
		// Tap location near A
		ivec2   B = A + (kDirection * delta);

		// Packed values
		vec4 blurInput = texelFetch(colorTexture, clamp(B, ivec2(0), textureSize(colorTexture, 0) - ivec2(1)), 0);

		// Signed kernel radius at this tap, in pixels
		float r_B = (blurInput.a * 2.0 - 1.0) * float(uMaxCoCRadiusPixels);
	

		/////////////////////////////////////////////////////////////////////////////////////////////
		// Compute blurry buffer

		float weight = 0.0;

		float wNormal =
			// Only consider mid- or background pixels (allows inpainting of the near-field)
			float(!inNearField(r_B)) *

			// Only blur B over A if B is closer to the viewer (allow 0.5 pixels of slop, and smooth the transition)
			// This term avoids "glowy" background objects but leads to aliasing under 4x downsampling
			// saturate( abs( r_A ) - abs( r_B ) + 1.5 ) *

			// Stretch the kernel extent to the radius at pixel B.
			kernel[clamp(int(float(abs(delta) * (KERNEL_TAPS - 1)) / (0.001 + abs(r_B * 0.8))), 0, KERNEL_TAPS)];

		weight = mix(wNormal, 1.0, nearFieldness_A);

		// far + mid-field output 
		blurWeightSum += weight;
		blurResult.rgb += blurInput.rgb * weight;

		///////////////////////////////////////////////////////////////////////////////////////////////
		// Compute near-field super blurry buffer

		vec4 nearInput;

		if (horizontal) {

			// On the first pass, extract coverage from the near field radius
			// Note that the near field gets a box-blur instead of a kernel 
			// blur; we found that the quality improvement was not worth the 
			// performance impact of performing the kernel computation here as well.

			// Curve the contribution based on the radius.  We tuned this particular
			// curve to blow out the near field while still providing a reasonable
			// transition into the focus field.
			nearInput.a = float(abs(delta) <= r_B) * clamp(r_B * uInvNearBlurRadiusPixels * 4.0, 0.0, 1.0);

			// Optionally increase edge fade contrast in the near field
			nearInput.a *= nearInput.a; nearInput.a *= nearInput.a;

			// Compute premultiplied-alpha color
			//nearInput.rgb = blurInput.rgb * nearInput.a;

		}

		else
		{
			nearInput = texelFetch(colorTexture, clamp(B, ivec2(0), textureSize(colorTexture, 0) - ivec2(1)), 0);
		}

	
		// We subsitute the following efficient expression for the more complex: weight = kernel[clamp(int(float(abs(delta) * (KERNEL_TAPS - 1)) * uInvNearBlurRadiusPixels), 0, KERNEL_TAPS)];
		weight = float(abs(delta) < uNearBlurRadiusPixels);
		nearResult += nearInput * weight;
		nearWeightSum += weight;
	}

	if (horizontal)
	{
		blurResult.a = packedA;
	}
	else
	{
		blurResult.a = 1.0;
	}


	// Normalize the blur
	blurResult.rgb /= blurWeightSum;
	nearResult /= max(nearWeightSum, 0.00001);

	//////////////////////////////////////////////
	vec4 color = texelFetch(colorTexture, ivec2(gl_FragCoord.xy), 0);

	vec3 normal = texelFetch(normalTexture, ivec2(gl_FragCoord.xy), 0).xyz;
	float depth = texelFetch(depthTexture, ivec2(gl_FragCoord.xy), 0).x;

	fragColor = color;
	gl_FragDepth = depth;

	//fragColor = blurResult;
}