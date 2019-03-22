#version 450
#extension GL_ARB_shading_language_include : require
#include "/defines.glsl"
#include "/globals.glsl"

layout(pixel_center_integer) in vec4 gl_FragCoord;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 modelViewProjectionMatrix;
uniform mat4 inverseModelViewProjectionMatrix;
uniform mat3 normalMatrix;
uniform float sharpness;
uniform uint coloring;
uniform bool environment;
uniform bool lens;

uniform vec3 lightPosition;
uniform vec3 diffuseMaterial;
uniform vec3 ambientMaterial;
uniform vec3 specularMaterial;
uniform float shininess;
uniform vec2 focusPosition;

uniform sampler2D positionTexture;
uniform sampler2D normalTexture;
uniform sampler2D environmentTexture;
uniform sampler2D bumpTexture;
uniform sampler2D materialTexture;
uniform usampler2D offsetTexture;
uniform sampler2D kernelDensityTexture;
uniform sampler2D scatterPlotTexture;

in vec4 gFragmentPosition;
out vec4 surfacePosition;
out vec4 surfaceNormal;
out vec4 surfaceDiffuse;
out vec4 sphereDiffuse;

//const float PI = 3.14159265359;

struct BufferEntry
{
	float near;
	float far;
	vec3 center;
	float radius;
	float value;
	uint previous;
};

layout(std430, binding = 1) buffer intersectionBuffer
{
	uint count;
	BufferEntry intersections[];
};

layout(std430, binding = 2) buffer statisticsBuffer
{
	uint intersectionCount;
	uint totalPixelCount;
	uint totalEntryCount;
	uint maximumEntryCount;
};

layout(std430, binding = 3) buffer depthRangeBuffer
{
	uint minDepth;
	uint maxDepth;

	uint minKernelDifference;
	uint maxKernelDifference;

	uint maxScatterPlotAlpha;
};

//struct Sphere
//{			
//	bool hit;
//	vec3 near;
//	vec3 far;
//	vec3 normal;
//};
//
//Sphere calcSphereIntersection(float r, vec3 origin, vec3 center, vec3 line)
//{
//	vec3 oc = origin - center;
//	vec3 l = normalize(line);
//	float loc = dot(l, oc);
//	float under_square_root = loc * loc - dot(oc, oc) + r*r;
//	if (under_square_root > 0)
//	{
//		float da = -loc + sqrt(under_square_root);
//		float ds = -loc - sqrt(under_square_root);
//
//		vec3 near = origin+min(da, ds) * l;
//		vec3 far = origin+max(da, ds) * l;
//		vec3 normal = (near - center);
//
//		return Sphere(true, near, far, normal);
//	}
//	else
//	{
//		return Sphere(false, vec3(0), vec3(0), vec3(0));
//	}
//}
//
//// from: https://www.ncbi.nlm.nih.gov/pmc/articles/PMC2839470/
//float gauss(vec3 x, vec3 c, float r, float s)
//{
//	vec3 xc = x-c;
//	float d2 = dot(xc,xc);
//	float s2 = s*s;
//	float r2 = r*r;
//
//	float eta = exp(-d2/(2.0*s2*r2) + 1.0/(2.0*s2));
//	return eta;
//}
//
//float gauss(float d, float r, float s)
//{
//	float d2 = d*d;
//	float s2 = s*s;
//	float r2 = r*r;
//
//	float eta = exp(-d2/(2.0*s2*r2) + 1.0/(2.0*s2));
//	return eta;
//}
//
//float blobby(float r, float R)
//{
//	if (r < R)
//		return 2.0*(r*r*r)/(R*R*R) - 3.0*(r*r)/(R*R) + 1.0;
//
//	return 0.0;
//}
//
float calcDepth(vec3 pos)
{
	float far = gl_DepthRange.far; 
	float near = gl_DepthRange.near;
	vec4 clip_space_pos = modelViewProjectionMatrix * vec4(pos, 1.0);
	float ndc_depth = clip_space_pos.z / clip_space_pos.w;
	return (((far - near) * ndc_depth) + near + far) / 2.0;
}
//
//mat3 rotationAlign( vec3 d, vec3 z )
//{
//    const vec3  v = cross( z, d );
//    const float c = dot( z, d );
//       const float k = 1.0f/(1.0f+c);
//
//    return mat3( v.x*v.x*k + c,     v.y*v.x*k - v.z,    v.z*v.x*k + v.y,
//                   v.x*v.y*k + v.z,   v.y*v.y*k + c,      v.z*v.y*k - v.x,
//                   v.x*v.z*k - v.y,   v.y*v.z*k + v.x,    v.z*v.z*k + c    );
//}
//
//float random(vec3 scale,float seed)
//{
//	return fract(sin(dot(gl_FragCoord.xyz+seed,scale))*43758.5453+seed);
//}
//
//float r2(vec3 pos, float scale)
//{
//	return fract(sin(dot(gl_FragCoord.xyz,vec3(scale,scale,scale)))*43758.5453);
//}
//
///*
//float smin(float a, float b, float k) {
//  float res = exp(-k * a) + exp(-k * b);
//  return -log(res) / k;
//}
//*/
//
//float smin(float a, float b, float k) {
//  float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
//  return mix(b, a, h) - k * h * (1.0 - h);
//}
//
//float torusDistance(vec3 p, vec2 t)
//{
//	vec2 q = vec2(length(p.xz)-t.x,p.y);
//	return length(q)-t.y;
//}
//
//float cylinderDistance( vec3 p, vec3 c )
//{
//  return length(p.xz)-c.z;
//}
//
//float cappedCylinderDistance( vec3 p, vec2 h )
//{
//  vec2 d = abs(vec2(length(p.xz),p.y)) - h;
//  return min(max(d.x,d.y),0.0) + length(max(d,0.0));
//}
//
//float sphereDistance( vec3 p, float s )
//{
//  return length(p)-s;
//}
//
//float subtract( float d1, float d2 )
//{
//    return max(-d1,d2);
//}
//
//float intersect( float d1, float d2 )
//{
//    return max(d1,d2);
//}
//
//float taperedCylinderDistance( in vec3 p, in vec3 c )
//{
//    vec2 q = vec2( length(p.xz), p.y );
//    vec2 v = vec2( c.z*c.y/c.x, -c.z );
//    vec2 w = v - q;
//    vec2 vv = vec2( dot(v,v), v.x*v.x );
//    vec2 qv = vec2( dot(v,w), v.x*w.x );
//    vec2 d = max(qv,0.0)*qv/vv;
//    return sqrt( dot(w,w) - max(d.x,d.y) ) * sign(max(q.y*v.x-q.x*v.y,w.y));
//}
//
//float blendDistance( vec3 p, vec2 t )
//{
//	float s = length(p);
//	float d = -torusDistance(p,t);
//	float is = t.y;
//	if (s > is)
//		d = max(d,s-is);
//	
//	return d;
//}
//
//
//bool intersectTorus( in vec3 ro, in vec3 rd, out vec3 i, in vec3 c, vec3 a, in vec2 r )
//{
//    vec3 x;
//	float maxd = 32.0;
//    float h = 1.0;
//    float t = 0.0;
//
//	mat3 m = rotationAlign(vec3(0.0,1.0,0.0),a);
//
//    for( int i=0; i<32; i++ )
//    {
//        if( h<0.001 || t>maxd )
//			break;
//	    
//		x = ro+rd*t;
//
//		float d = blendDistance( (x-c)*m, r);
//
//		h = (d);
//        t += h;
//    }
//    
//	if( t<maxd )
//	{
//		i = x;
//		return true;
//	}
//
//	return false;
//}
//
//void swap (inout uint a, inout uint b)
//{
//	uint t = a;
//	a = b;
//	b = t;
//}
//
//
//void sort2 (in uint aa, inout uint a, in uint bb, inout uint b)
//{
//	if (aa > bb)	
//	{
//		swap(a,b);
//	}
//}
//
//
//void sort3 (uint aa, inout uint a, uint bb, inout uint b, uint cc, inout uint c)
//{
//	if(aa>bb)
//	{
//		swap(aa,bb);
//		swap(a,b);
//	}
//	if(aa>cc)
//	{
//		swap(aa,cc);
//		swap(a,c);
//	}
//	if(bb>cc)
//	{
//		swap(bb,cc);
//		swap(b,c);
//	}
//}
//
//bool intersectTorus2( in vec3 ro, in vec3 rd, out vec3 i, in vec3 c, vec3 a, in vec2 r )
//{	
//	if (intersectTorus(ro,rd,i,c,a,r))
//	{
//		return intersectTorus(i,rd,i,c,a,r);
//	}
//
//	return false;
//}
//
// From http://http.developer.nvidia.com/GPUGems/gpugems_ch17.html
//vec2 latlong(vec3 v)
//{
//	v = normalize(v);
//	float theta = acos(v.z) / 2; // +z is up
//	float phi = atan(v.y, v.x) + 3.1415926535897932384626433832795;
//	return vec2(phi, theta) * vec2(0.1591549, 0.6366198);
//}
//
//vec4 shade(vec3 ambientColor, vec3 diffuseColor, vec3 specularColor, float shininess, vec3 position, vec3 normal, vec3 light, vec3 view)
//{
//	vec3 N = normalize(normal);
//	vec3 L = normalize(light.xyz-position.xyz);
//	vec3 R = normalize(reflect(L, N));
//	float NdotL = max(0.0,dot(N, L));
//	float RdotV = max(0.0,dot(R, view));
//	
//	vec4 environmentColor = vec4(1.0,1.0,1.0,1.0);	
//
//	if (environment)
//	{
//		vec2 uv = latlong(R);
//		environmentColor = mix(texture(environmentTexture,uv),vec4(1.0),0.5);
//	}
//
//	vec3 color = ambientColor + NdotL * diffuseColor + pow(RdotV,shininess) * specularColor * environmentColor.rgb;
//	return vec4(color,1.0);
//}
//
//mat3 cotangentFrame(vec3 N, vec3 p, vec2 uv)
//{
//  // get edge vectors of the pixel triangle
//  vec3 dp1 = dFdx(p);
//  vec3 dp2 = dFdy(p);
//  vec2 duv1 = dFdx(uv);
//  vec2 duv2 = dFdy(uv);
//
//  // solve the linear system
//  vec3 dp2perp = cross(dp2, N);
//  vec3 dp1perp = cross(N, dp1);
//  vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
//  vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
//
//  // construct a scale-invariant frame 
//  float invmax = 1.0 / sqrt(max(dot(T,T), dot(B,B)));
//  return mat3(normalize(T * invmax), normalize(B * invmax), N);
//}
//
//vec3 perturb(vec3 map, vec3 N, vec3 V, vec2 texcoord)
//{
//  mat3 TBN = cotangentFrame(N, -V, texcoord);
//  return normalize(TBN * map);
//}
//
//#define MAGIC_ANGLE 0.868734829276 // radians
//
//const float warp_theta = MAGIC_ANGLE;
//float tan_warp_theta = tan(warp_theta);
//
///* Return a permutation matrix whose first two columns are u and v basis 
//   vectors for a cube face, and whose third column indicates which axis 
//   (x,y,z) is maximal. */
//mat3 getPT(in vec3 p) {
//
//    vec3 a = abs(p);
//    float c = max(max(a.x, a.y), a.z);    
//    vec3 s = c == a.x ? vec3(1.,0,0) : c == a.y ? vec3(0,1.,0) : vec3(0,0,1.);
//    s *= sign(dot(p, s));
//    vec3 q = s.yzx;
//    return mat3(cross(q,s), q, s);
//
//}
//
///* For any point in 3D, obtain the permutation matrix, as well as grid coordinates
//   on a cube face. */
//void posToGrid(in float N, in vec3 pos, out mat3 PT, out vec2 g) {
//    
//    // Get permutation matrix and cube face id
//    PT = getPT(pos);
//    
//    // Project to cube face
//    vec3 c = pos * PT;     
//    vec2 p = c.xy / c.z;      
//    
//    // Unwarp through arctan function
//    vec2 q = atan(p*tan_warp_theta)/warp_theta; 
//    
//    // Map [-1,1] interval to [0,N] interval
//    g = (q*0.5 + 0.5)*N;
//    
//}
//
//vec2 warp(vec2 x)
//{
//	return atan(fract(x) * 1.18228668555) * 1.151099238;
//}
//
//vec3 PerturbNormal  (vec3 surf_pos, vec3 surf_norm, float height)
//{
//	vec3 vSigmaS = dFdx(surf_pos);
//	vec3 vSigmaT = dFdy(surf_pos);
//	vec3 vN = surf_norm;//normalized
//	vec3 vR1 = cross(vSigmaT,vN);
//	vec3 vR2 = cross(vN,vSigmaS);
//	float fDet = dot(vSigmaS,vR1);
//	float dBs = dFdxFine(height);
//	float dBt = dFdyFine(height);
//	vec3 vSurfGrad = sign(fDet)*(dBs*vR1+dBt*vR2);
//	return normalize(abs(fDet)*vN-vSurfGrad);
//}
//
//vec3 rnmBlendUnpacked(vec3 n1, vec3 n2)
//{
//    n1 += vec3( 0,  0, 1);
//    n2 *= vec3(-1, -1, 1);
//    return n1*dot(n1, n2)/n1.z - n2;
//}

// https://stackoverflow.com/questions/49640250/calculate-normals-from-heightmap
vec3 calculateNormal(ivec2 texelCoord){
	
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
//	uint offset = texelFetch(offsetTexture,ivec2(gl_FragCoord.xy),0).r;
//
//	if (offset == 0)
//		discard;
//
//
//	vec4 position = texelFetch(positionTexture,ivec2(gl_FragCoord.xy),0);
//	vec4 normal = texelFetch(normalTexture,ivec2(gl_FragCoord.xy),0);
//	/*
//	surfaceDiffuse = vec4(1.0,0.0,0.0,1.0);
//	surfaceDiffuse.xyz += normal.xyz;
//	return;
//	*/
//	//vec4 depth = texelFetch(depthTexture,ivec2(gl_FragCoord.xy),0);
//
//	vec4 fragCoord = gFragmentPosition;
//	fragCoord /= fragCoord.w;
//	
//	vec4 near = inverseModelViewProjectionMatrix*vec4(fragCoord.xy,-1.0,1.0);
//	near /= near.w;
//
//	vec4 far = inverseModelViewProjectionMatrix*vec4(fragCoord.xy,1.0,1.0);
//	far /= far.w;
//
//	vec3 V = normalize(far.xyz-near.xyz);
//
//	//float noise = ( .5 - random( vec3( 1. ), length( gl_FragCoord ) ) );
//	//near.xyz += V*noise;
//
//	const uint maxEntries = 128;
//	uint entryCount = 0;
//	uint indices[maxEntries];
//
//	while (offset > 0)
//	{
//		indices[entryCount++] = offset;
//		offset = intersections[offset].previous;
//	}
//	/*
//	if (entryCount <= 3)
//	{
//		fragColor = vec4(0.0,0.0,1.0,1.0);
//		return;
//	}*/
//
//	if (entryCount == 0)
//		discard;
//
//	vec4 closestPosition = position;//vec4(far.xyz,65535.0);// position;
//	vec3 closestNormal = normal.xyz;
//
//	float sharpnessFactor = 1.0;
//	vec3 ambientColor = ambientMaterial;
//	vec3 diffuseColor = vec3(1.0,1.0,1.0);
//	vec3 specularColor = specularMaterial;
//	vec3 diffuseSphereColor = vec3(1.0,1.0,1.0);
//
////#define LENSING
////#define COLORING
//
//#ifdef COLORING
//	if (coloring > 0)
//	{
//		diffuseColor = vec3(normal.w,1.0,1.0);
//		diffuseSphereColor = diffuseColor;
//	}
//#endif
//
//
//#ifdef LENSING
//	float focusFactor = 0.0;
//	if (lens)
//	{
//		focusFactor = min(16.0,1.0/(16.0*pow(length((fragCoord.xy-focusPosition)/vec2(0.5625,1.0)),2.0)));
//		sharpnessFactor += focusFactor;
//		focusFactor = min(1.0,focusFactor);
//		//ambientColor = mix(vec3(0.0),ambientColor,focusFactor);
//	}
//#endif
//
//	uint startIndex = 0;
//
//	// selection sort
//	for(uint currentIndex = 0; currentIndex < entryCount/* - 1*/; currentIndex++)
//	{
//		uint minimumIndex = currentIndex;
//
//		for(uint i = currentIndex+1; i < entryCount; i++)
//		{
//			if(intersections[indices[i]].near < intersections[indices[minimumIndex]].near)
//			{
//				minimumIndex = i;					
//			}
//		}
//
//		if (minimumIndex != currentIndex)
//		{
//			uint temp = indices[minimumIndex];
//			indices[minimumIndex] = indices[currentIndex];
//			indices[currentIndex] = temp;
//		}
//
//		if (startIndex < currentIndex)
//		{
//			uint endIndex = currentIndex;
//
//			if (currentIndex >= entryCount-1 || intersections[indices[startIndex]].far < intersections[indices[currentIndex]].near)
//			{
//				const uint maximumSteps = 128;
//				const float s = sharpness*sharpnessFactor;
//
//				uint ii = indices[startIndex+1];
//				float nearDistance = intersections[ii].near;
//				float farDistance = intersections[indices[endIndex-1]].far;
//				/*
//				if (entryCount == 1)
//				{
//					nearDistance = intersections[indices[0]].near;
//					farDistance = intersections[indices[0]].far;
//					//fragColor = vec4(0.0,1.0,1.0,1.0);
//					//return;
//				}*/
//
//				float maximumDistance = (farDistance-nearDistance)+32.0;
//				float surfaceDistance = 1.0;
//
//				const float eps = 0.0125;
//				vec4 rayOrigin = vec4(near.xyz+V*nearDistance,nearDistance);
//				vec4 rayDirection = vec4(V,1.0);
//				vec4 currentPosition;
//				
//				vec4 candidatePosition = rayOrigin;
//				vec3 candidateNormal = vec3(0.0);
//				vec3 candidateColor = vec3(0.0);
//				float candidateValue = 0.0;
//
//				float minimumDistance = maximumDistance;
//
//				uint currentStep = 0;			
//				float t = 0.0;
//
//				while (++currentStep <= maximumSteps && t <= maximumDistance)
//				{    
//					currentPosition = rayOrigin + rayDirection*t;
//
//					if (currentPosition.w > closestPosition.w)
//						break;
//
//					float sumValue = 0.0;
//					vec3 sumNormal = vec3(0.0);
//					vec3 sumColor = vec3(0.0);
//					vec3 sumPattern = vec3(0.0);
//					float sumDisplacement = 0.0;
//					
//					for (uint j = startIndex; j <= endIndex; j++)
//					{
//						uint ij = indices[j];
//
//						vec3 aj = intersections[ij].center;
//						float rj = intersections[ij].radius;
//						vec3 cj = vec3(intersections[ij].value,1.0,1.0);
//
//
//#ifdef LENSING
//						cj = mix(vec3(diffuseMaterial),cj,focusFactor);
//#endif
//
///*
//						vec3 n0 = normalize(atomOffset);
//
//
//						float delta = 0.0625;//625;//125;
//						mat3 PT;
//						vec2 uv;
//						posToGrid(PI,n0,PT,uv);
//						float frequency = 4.0;
//						float amplitude = 0.0625;
//						float exponent = 1.0;
//						uv *= frequency;
//						float displacement = amplitude*(1.0+pow(cos(uv.x)*cos(uv.y),exponent));
//
//						vec2 uvx,uvy,uvz;
//						posToGrid(PI,normalize(n0+vec3(1.0,0.0,0.0)*delta),PT,uvx);
//						uvx *= frequency;
//						posToGrid(PI,normalize(n0+vec3(0.0,1.0,0.0)*delta),PT,uvy);
//						uvy *= frequency;
//						posToGrid(PI,normalize(n0+vec3(0.0,0.0,1.0)*delta),PT,uvz);
//						uvz *= frequency;
//
//						float dpx = amplitude*(1.0+pow(cos(uvx.x)*cos(uvx.y),exponent));
//						float dpy = amplitude*(1.0+pow(cos(uvy.x)*cos(uvy.y),exponent));
//						float dpz = amplitude*(1.0+pow(cos(uvz.x)*cos(uvz.y),exponent));
//
//
//
//
//						float fl = length(atomOffset);
//						//atomOffset = atomOffset - n0*displacement;
//						
//						vec3 g = 8.0*(vec3(dpx,dpy,dpz)-vec3(displacement));
//						//vec3 g = vec3( -amplitude*sin(uv.x)*cos(uv.y), -amplitude*cos(uv.x)*sin(uv.y), displacement);
//						vec3 gp = dot(g,n0)*n0;
//						vec3 gt = g - gp;
//						//n0 = n0 - gt;
//						
//						//sumDisplacement += (atomNormal-gt);
//						//atomNormal = atomNormal+gt;
//
//						//sumDisplacement += displacement;
//*/
//						vec3 atomOffset = currentPosition.xyz-aj;
//						float atomDistance = length(atomOffset)/rj;
//
//						float atomValue = exp(-s*atomDistance*atomDistance);//exp(-(ad*ad)/(2.0*s*s*rj*rj)+0.5/(s*s));
//						//vec3 atomNormal = 2.0*s*atomOffset*atomValue / (rj*rj);
//						vec3 atomNormal = atomValue*normalize(atomOffset);
//						//vec3 atomNormal = atomValue*normalize(n0);
//#ifdef COLORING
//						vec3 atomColor = cj*atomValue;//*mix(vec3(1.0),vec3(0.0,0.0,1.0),2.0*displacement);
//#endif
//						
//						//float u = theta;
//						//float v = phi;
//						//float u = (6.0/PI)*atan(theta/sqrt(theta*theta+2.0));
//						//float v = (phi*sqrt(theta*theta+2.0)) / (theta*theta+phi*phi+1.0);
//
//						//void posToGrid(in float N, in vec3 pos, out mat3 PT, out vec2 g) {
//						
//						//atomNormal *= atomValue;
//						//float dX = s
//
//						//atomValue -= cos(4.0*uv.x)*cos(4.0*uv.y)*0.001;
//
//
//						sumValue += atomValue;
//						sumNormal += atomNormal;
//#ifdef COLORING
//						if (coloring > 0)
//							sumColor += atomColor;
//#endif
//					}
//					
//					//surfaceDistance = (-sumValue+exp(-s));///float(cnt);//1.0;//sqrt(-log(sumValue) / (s))-1.0;//-sumDisplacement;
//					surfaceDistance = sqrt(-log(sumValue) / (s))-1.0;//-sumDisplacement;
//
//
//					if (surfaceDistance < eps)
//					{
//						if (currentPosition.w <= closestPosition.w)
//						{
//							closestPosition = currentPosition;
//							closestNormal = sumNormal;
//							//closestNormal += sumDisplacement;
//#ifdef COLORING
//							if (coloring > 0)
//							{
//								diffuseColor = sumColor / sumValue;
//							}
//
//
//#endif
//						}
//						break;
//					}
//
//					if (surfaceDistance < minimumDistance)
//					{
//						minimumDistance = surfaceDistance;
//						candidatePosition = currentPosition;
//						candidateNormal = sumNormal;
//						candidateColor = sumColor;
//						candidateValue = sumValue;
//					}
//
//					t += surfaceDistance*1.2;
//				}
//				
//				if (currentStep > maximumSteps)
//				{
//					if (candidatePosition.w <= closestPosition.w)
//					{
//						closestPosition = candidatePosition;
//						closestNormal = candidateNormal;
//
//						if (coloring > 0)
//							diffuseColor = candidateColor / candidateValue;
//					}
//				}
//
//				startIndex++;
//			}
//
//
//		}
//	}
//	/*
//	if (closestPosition == position)
//	{
//		if (entryCount == 0)
//		{
//			fragColor = vec4(1.0,1.0,1.0,1.0);
//			return;
//		}
//		else if (entryCount == 1)
//		{
//			fragColor = vec4(0.0,0.0,1.0,1.0);
//		}
//		else if (entryCount == 2)
//		{
//			fragColor = vec4(0.0,1.0,0.0,1.0);
//		}
//		else if (entryCount == 3)
//		{
//			fragColor = vec4(1.0,0.0,0.0,1.0);
//		}
//		else
//		{
//			fragColor = vec4(1.0/float(entryCount),1.0/float(entryCount),0.0,1.0);
//		}
//	}*/
//
//	if (closestPosition.w >= 65535.0f)
//		discard;
//
//#ifdef NORMAL		
//	vec3 N = normalize(closestNormal);
//	
//	vec3 blend = abs( N );
//	blend = normalize(max(blend, 0.00001)); // Force weights to sum to 1.0
//	float b = (blend.x + blend.y + blend.z);
//	blend /= vec3(b, b, b);	
//	
//	// https://medium.com/@bgolus/normal-mapping-for-a-triplanar-shader-10bf39dca05a
//	/*
//	vec3 blend = abs(normal.xyz);
//	blend = max(blend - vec3(0.2), vec3(0));
//	blend /= dot(blend, vec3(1,1,1));*/
//	
///*
//	vec2 uvX = warp(closestPosition.zy*0.5);
//	vec2 uvY = warp(closestPosition.xz*0.5);
//	vec2 uvZ = warp(closestPosition.xy*0.5);
//*/
//	vec2 uvX = closestPosition.zy*0.5;
//	vec2 uvY = closestPosition.xz*0.5;
//	vec2 uvZ = closestPosition.xy*0.5;
//
//	vec3 normalX = 2.0*texture(bumpTexture,uvX).xyz - 1.0;
//	vec3 normalY = 2.0*texture(bumpTexture,uvY).xyz - 1.0;
//	vec3 normalZ = 2.0*texture(bumpTexture,uvZ).xyz - 1.0;
//
//	normalX = vec3(0.0, normalX.yx);
//	normalY = vec3(normalY.x, 0.0, normalY.y);
//	normalZ = vec3(normalZ.xy, 0.0);
//
//	// Swizzle world normals into tangent space and apply Whiteout blend
///*
//	normalX = vec3(normalX.xy + N.zy,abs(normalX.z) * N.x );
//	normalY = vec3(normalY.xy + N.xz,abs(normalY.z) * N.y);
//	normalZ = vec3(normalZ.xy + N.xy,abs(normalZ.z) * N.z);
//*/
//
//
///*
//	// Get absolute value of normal to ensure positive tangent "z" for blend
//	vec3 absVertNormal = abs(N);
//	
//	// Swizzle world normals to match tangent space and apply RNM blend
//	normalX = rnmBlendUnpacked(vec3(N.zy, absVertNormal.x), normalX);
//	normalY = rnmBlendUnpacked(vec3(N.xz, absVertNormal.y), normalY);
//	normalZ = rnmBlendUnpacked(vec3(N.xy, absVertNormal.z), normalZ);
//
//	vec3 axisSign = sign(N);
//	
//	// Reapply sign to Z
//	normalX.z *= axisSign.x;
//	normalY.z *= axisSign.y;
//	normalZ.z *= axisSign.z;
//*/	
//	vec3 worldNormal = normalize(N + normalX.xyz * blend.x + normalY.xyz * blend.y + normalZ.xyz * blend.z);
//		
// // Finally, blend the results of the 3 planar projections.
//
//
//	// Apply bump vector to vertex-interpolated normal vector.
//	closestNormal = worldNormal;
//#endif
//
//	vec4 cp = modelViewMatrix*vec4(closestPosition.xyz, 1.0);
//	cp = cp / cp.w;
//
//	surfacePosition = closestPosition;
///*
//	vec3 cols[8]= {
//		vec3(255,255,255)/255.0,
//		vec3(255,255,178)/255.0,
//		vec3(254,217,118)/255.0,
//		vec3(254,178,76)/255.0,
//		vec3(253,141,60)/255.0,
//		vec3(252,78,42)/255.0,
//		vec3(227,26,28)/255.0,
//		vec3(177,0,38)/255.0,
//	};
//*/
//
//
//
//
//	//diffuseColor.rgb = cols[min(entryCount/2 + entryCount%2,7)];
//	//diffuseColor.rgb = vec3(float(entryCount)/32.0);
//
//	closestNormal.xyz = normalMatrix*closestNormal.xyz;
//	closestNormal.xyz = normalize(closestNormal.xyz);
//	surfaceNormal = vec4(closestNormal.xyz,cp.z);
//
//#ifdef MATERIAL
//
//	vec3 materialColor = texture( materialTexture , closestNormal.xy*0.5+0.5 ).rgb;
//	//materialColor = vec3( 1. ) - ( vec3( 1. ) - materialColor ) * ( vec3( 1. ) - materialColor );
//	diffuseColor *= materialColor;
//
//#endif
//
//	surfaceDiffuse = vec4(diffuseColor,1.0);
//	sphereDiffuse = vec4(diffuseSphereColor,1.0);
//	gl_FragDepth = calcDepth(closestPosition.xyz);
//
//#ifdef STATISTICS
//	intersectionCount = count;
//	atomicAdd(totalPixelCount,1);
//	atomicAdd(totalEntryCount,entryCount);
//	atomicMax(maximumEntryCount,entryCount);
//#endif

	//const float s = sharpness*sharpnessFactor;

	// apply density estimation
	float kernelDensity = texelFetch(kernelDensityTexture,ivec2(gl_FragCoord.xy),0).r;

	if(kernelDensity == 0.0f)
		discard;

	vec4 position = texelFetch(positionTexture,ivec2(gl_FragCoord.xy),0);

	surfacePosition = vec4(position.xyz, 1);
	surfacePosition.z = kernelDensity;

	surfaceNormal = vec4(calculateNormal(ivec2(gl_FragCoord.xy)), 1);
	//surfaceNormal *= -1.0;

	// set diffuse colors
	surfaceDiffuse = vec4(1.0f, 1.0f, 1.0f, 1.0f); 
	sphereDiffuse = vec4(1.0f, 1.0f, 1.0f, 1.0f); 

	gl_FragDepth = calcDepth(surfacePosition.xyz);

#ifdef COLORMAP
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
		vec4 posViewSpace = modelViewMatrix*surfacePosition;
		float cameraDistance = abs(dot(posViewSpace.xyz,V));

		// floatBitsToUint: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/floatBitsToInt.xhtml
		uint uintDepth = floatBitsToUint(cameraDistance);
		
		// identify depth-range
		atomicMin(minDepth, uintDepth);
		atomicMax(maxDepth, uintDepth);
	}	
#endif
		
#ifdef DISTANCEBLENDING
	//float kernelValue = texelFetch(kernelDensityTexture,ivec2(gl_FragCoord.xy),0).r;
	float kernelDifference = texelFetch(kernelDensityTexture,ivec2(gl_FragCoord.xy),0).g;

	// identify KDE-difference-range
	atomicMin(minKernelDifference, floatBitsToUint(kernelDifference));
	atomicMax(maxKernelDifference, floatBitsToUint(kernelDifference));
#endif

	// find maximum alpha after additive blending
	float alpha = texelFetch(scatterPlotTexture,ivec2(gl_FragCoord.xy),0).a;
	atomicMax(maxScatterPlotAlpha, floatBitsToUint(alpha));
}