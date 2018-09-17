#version 450
#extension GL_ARB_shading_language_include : require
#include "/defines.glsl"
#include "/globals.glsl"

layout(pixel_center_integer) in vec4 gl_FragCoord;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat4 inverseProjection;
uniform mat4 modelViewProjection;
uniform mat4 inverseModelViewProjection;
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
uniform sampler2D materialTexture;
uniform usampler2D offsetTexture;

in vec4 gFragmentPosition;
out vec4 surfacePosition;
out vec4 surfaceNormal;
out vec4 surfaceDiffuse;
out vec4 sphereDiffuse;

const float PI = 3.14159265359;

struct Element
{
	vec3 color;
	float radius;
};

struct Residue
{
	vec4 color;
};

struct Chain
{
	vec4 color;
};

layout(std140, binding = 0) uniform elementBlock
{
	Element elements[32];
};

layout(std140, binding = 1) uniform residueBlock
{
	Residue residues[32];
};

layout(std140, binding = 2) uniform chainBlock
{
	Chain chains[64];
};

struct BufferEntry
{
	float near;
	float far;
	vec3 center;
	uint id;
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

struct Sphere
{			
	bool hit;
	vec3 near;
	vec3 far;
	vec3 normal;
};

Sphere calcSphereIntersection(float r, vec3 origin, vec3 center, vec3 line)
{
	vec3 oc = origin - center;
	vec3 l = normalize(line);
	float loc = dot(l, oc);
	float under_square_root = loc * loc - dot(oc, oc) + r*r;
	if (under_square_root > 0)
	{
		float da = -loc + sqrt(under_square_root);
		float ds = -loc - sqrt(under_square_root);

		vec3 near = origin+min(da, ds) * l;
		vec3 far = origin+max(da, ds) * l;
		vec3 normal = (near - center);

		return Sphere(true, near, far, normal);
	}
	else
	{
		return Sphere(false, vec3(0), vec3(0), vec3(0));
	}
}

// from: https://www.ncbi.nlm.nih.gov/pmc/articles/PMC2839470/
float gauss(vec3 x, vec3 c, float r, float s)
{
	vec3 xc = x-c;
	float d2 = dot(xc,xc);
	float s2 = s*s;
	float r2 = r*r;

	float eta = exp(-d2/(2.0*s2*r2) + 1.0/(2.0*s2));
	return eta;
}

float gauss(float d, float r, float s)
{
	float d2 = d*d;
	float s2 = s*s;
	float r2 = r*r;

	float eta = exp(-d2/(2.0*s2*r2) + 1.0/(2.0*s2));
	return eta;
}

float blobby(float r, float R)
{
	if (r < R)
		return 2.0*(r*r*r)/(R*R*R) - 3.0*(r*r)/(R*R) + 1.0;

	return 0.0;
}

float calcDepth(vec3 pos)
{
	float far = gl_DepthRange.far; 
	float near = gl_DepthRange.near;
	vec4 clip_space_pos = modelViewProjection * vec4(pos, 1.0);
	float ndc_depth = clip_space_pos.z / clip_space_pos.w;
	return (((far - near) * ndc_depth) + near + far) / 2.0;
}

mat3 rotationAlign( vec3 d, vec3 z )
{
    const vec3  v = cross( z, d );
    const float c = dot( z, d );
       const float k = 1.0f/(1.0f+c);

    return mat3( v.x*v.x*k + c,     v.y*v.x*k - v.z,    v.z*v.x*k + v.y,
                   v.x*v.y*k + v.z,   v.y*v.y*k + c,      v.z*v.y*k - v.x,
                   v.x*v.z*k - v.y,   v.y*v.z*k + v.x,    v.z*v.z*k + c    );
}

float random(vec3 scale,float seed)
{
	return fract(sin(dot(gl_FragCoord.xyz+seed,scale))*43758.5453+seed);
}

float r2(vec3 pos, float scale)
{
	return fract(sin(dot(gl_FragCoord.xyz,vec3(scale,scale,scale)))*43758.5453);
}

/*
float smin(float a, float b, float k) {
  float res = exp(-k * a) + exp(-k * b);
  return -log(res) / k;
}
*/

float smin(float a, float b, float k) {
  float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
  return mix(b, a, h) - k * h * (1.0 - h);
}

float torusDistance(vec3 p, vec2 t)
{
	vec2 q = vec2(length(p.xz)-t.x,p.y);
	return length(q)-t.y;
}

float cylinderDistance( vec3 p, vec3 c )
{
  return length(p.xz)-c.z;
}

float cappedCylinderDistance( vec3 p, vec2 h )
{
  vec2 d = abs(vec2(length(p.xz),p.y)) - h;
  return min(max(d.x,d.y),0.0) + length(max(d,0.0));
}

float sphereDistance( vec3 p, float s )
{
  return length(p)-s;
}

float subtract( float d1, float d2 )
{
    return max(-d1,d2);
}

float intersect( float d1, float d2 )
{
    return max(d1,d2);
}

float taperedCylinderDistance( in vec3 p, in vec3 c )
{
    vec2 q = vec2( length(p.xz), p.y );
    vec2 v = vec2( c.z*c.y/c.x, -c.z );
    vec2 w = v - q;
    vec2 vv = vec2( dot(v,v), v.x*v.x );
    vec2 qv = vec2( dot(v,w), v.x*w.x );
    vec2 d = max(qv,0.0)*qv/vv;
    return sqrt( dot(w,w) - max(d.x,d.y) ) * sign(max(q.y*v.x-q.x*v.y,w.y));
}

float blendDistance( vec3 p, vec2 t )
{
	float s = length(p);
	float d = -torusDistance(p,t);
	float is = t.y;
	if (s > is)
		d = max(d,s-is);
	
	return d;
}


bool intersectTorus( in vec3 ro, in vec3 rd, out vec3 i, in vec3 c, vec3 a, in vec2 r )
{
    vec3 x;
	float maxd = 32.0;
    float h = 1.0;
    float t = 0.0;

	mat3 m = rotationAlign(vec3(0.0,1.0,0.0),a);

    for( int i=0; i<32; i++ )
    {
        if( h<0.001 || t>maxd )
			break;
	    
		x = ro+rd*t;

		float d = blendDistance( (x-c)*m, r);

		h = (d);
        t += h;
    }
    
	if( t<maxd )
	{
		i = x;
		return true;
	}

	return false;
}

void swap (inout uint a, inout uint b)
{
	uint t = a;
	a = b;
	b = t;
}


void sort2 (in uint aa, inout uint a, in uint bb, inout uint b)
{
	if (aa > bb)	
	{
		swap(a,b);
	}
}


void sort3 (uint aa, inout uint a, uint bb, inout uint b, uint cc, inout uint c)
{
	if(aa>bb)
	{
		swap(aa,bb);
		swap(a,b);
	}
	if(aa>cc)
	{
		swap(aa,cc);
		swap(a,c);
	}
	if(bb>cc)
	{
		swap(bb,cc);
		swap(b,c);
	}
}

bool intersectTorus2( in vec3 ro, in vec3 rd, out vec3 i, in vec3 c, vec3 a, in vec2 r )
{	
	if (intersectTorus(ro,rd,i,c,a,r))
	{
		return intersectTorus(i,rd,i,c,a,r);
	}

	return false;
}

// From http://http.developer.nvidia.com/GPUGems/gpugems_ch17.html
vec2 latlong(vec3 v)
{
	v = normalize(v);
	float theta = acos(v.z) / 2; // +z is up
	float phi = atan(v.y, v.x) + 3.1415926535897932384626433832795;
	return vec2(phi, theta) * vec2(0.1591549, 0.6366198);
}

vec4 shade(vec3 ambientColor, vec3 diffuseColor, vec3 specularColor, float shininess, vec3 position, vec3 normal, vec3 light, vec3 view)
{
	vec3 N = normalize(normal);
	vec3 L = normalize(light.xyz-position.xyz);
	vec3 R = normalize(reflect(L, N));
	float NdotL = max(0.0,dot(N, L));
	float RdotV = max(0.0,dot(R, view));
	
	vec4 environmentColor = vec4(1.0,1.0,1.0,1.0);	

	if (environment)
	{
		vec2 uv = latlong(R);
		environmentColor = mix(texture(environmentTexture,uv),vec4(1.0),0.5);
	}

	vec3 color = ambientColor + NdotL * diffuseColor + pow(RdotV,shininess) * specularColor * environmentColor.rgb;
	return vec4(color,1.0);
}

#define MAGIC_ANGLE 0.868734829276 // radians

const float warp_theta = MAGIC_ANGLE;
float tan_warp_theta = tan(warp_theta);

/* Return a permutation matrix whose first two columns are u and v basis 
   vectors for a cube face, and whose third column indicates which axis 
   (x,y,z) is maximal. */
mat3 getPT(in vec3 p) {

    vec3 a = abs(p);
    float c = max(max(a.x, a.y), a.z);    
    vec3 s = c == a.x ? vec3(1.,0,0) : c == a.y ? vec3(0,1.,0) : vec3(0,0,1.);
    s *= sign(dot(p, s));
    vec3 q = s.yzx;
    return mat3(cross(q,s), q, s);

}

/* For any point in 3D, obtain the permutation matrix, as well as grid coordinates
   on a cube face. */
void posToGrid(in float N, in vec3 pos, out mat3 PT, out vec2 g) {
    
    // Get permutation matrix and cube face id
    PT = getPT(pos);
    
    // Project to cube face
    vec3 c = pos * PT;     
    vec2 p = c.xy / c.z;      
    
    // Unwarp through arctan function
    vec2 q = atan(p*tan_warp_theta)/warp_theta; 
    
    // Map [-1,1] interval to [0,N] interval
    g = (q*0.5 + 0.5)*N;
    
}

void main()
{
	uint offset = texelFetch(offsetTexture,ivec2(gl_FragCoord.xy),0).r;

	if (offset == 0)
		discard;

	vec4 position = texelFetch(positionTexture,ivec2(gl_FragCoord.xy),0);
	vec4 normal = texelFetch(normalTexture,ivec2(gl_FragCoord.xy),0);

	//vec4 depth = texelFetch(depthTexture,ivec2(gl_FragCoord.xy),0);

	vec4 fragCoord = gFragmentPosition;
	fragCoord /= fragCoord.w;
	
	vec4 near = inverseModelViewProjection*vec4(fragCoord.xy,-1.0,1.0);
	near /= near.w;

	vec4 far = inverseModelViewProjection*vec4(fragCoord.xy,1.0,1.0);
	far /= far.w;

	vec3 V = normalize(far.xyz-near.xyz);

	//float noise = ( .5 - random( vec3( 1. ), length( gl_FragCoord ) ) );
	//near.xyz += V*noise;

	const uint maxEntries = 128;
	uint entryCount = 0;
	uint indices[maxEntries];

	while (offset > 0)
	{
		indices[entryCount++] = offset;
		offset = intersections[offset].previous;
	}
	/*
	if (entryCount <= 3)
	{
		fragColor = vec4(0.0,0.0,1.0,1.0);
		return;
	}*/

	if (entryCount == 0)
		discard;

	vec4 closestPosition = position;//vec4(far.xyz,65535.0);// position;
	vec3 closestNormal = normal.xyz;

	float sharpnessFactor = 1.0;
	vec3 ambientColor = ambientMaterial;
	vec3 diffuseColor = vec3(1.0,1.0,1.0);
	vec3 specularColor = specularMaterial;
	vec3 diffuseSphereColor = vec3(1.0,1.0,1.0);

//#define LENSING
//#define COLORING

#ifdef COLORING
	if (coloring > 0)
	{
		uint id = floatBitsToUint(normal.w);
		uint elementId = bitfieldExtract(id,0,8);
		uint residueId = bitfieldExtract(id,8,8);
		uint chainId = bitfieldExtract(id,16,8);

		if (coloring == 1)
			diffuseColor = elements[elementId].color.rgb;
		else if (coloring == 2)
			diffuseColor = residues[residueId].color.rgb;
		else if (coloring == 3)
			diffuseColor = chains[chainId].color.rgb;

		diffuseSphereColor = diffuseColor;
	}
#endif


#ifdef LENSING
	float focusFactor = 0.0;
	if (lens)
	{
		focusFactor = min(16.0,1.0/(16.0*pow(length((fragCoord.xy-focusPosition)/vec2(0.5625,1.0)),2.0)));
		sharpnessFactor += focusFactor;
		focusFactor = min(1.0,focusFactor);
		//ambientColor = mix(vec3(0.0),ambientColor,focusFactor);
	}
#endif

	uint startIndex = 0;

	// selection sort
	for(uint currentIndex = 0; currentIndex < entryCount/* - 1*/; currentIndex++)
	{
		uint minimumIndex = currentIndex;

		for(uint i = currentIndex+1; i < entryCount; i++)
		{
			if(intersections[indices[i]].near < intersections[indices[minimumIndex]].near)
			{
				minimumIndex = i;					
			}
		}

		if (minimumIndex != currentIndex)
		{
			uint temp = indices[minimumIndex];
			indices[minimumIndex] = indices[currentIndex];
			indices[currentIndex] = temp;
		}

		if (startIndex <= currentIndex)
		{
			uint endIndex = currentIndex;

			if (currentIndex >= entryCount-1 || intersections[indices[startIndex]].far < intersections[indices[currentIndex]].near)
			{
				const uint maximumSteps = 24;
				const float s = sharpness*sharpnessFactor;

				uint ii = indices[startIndex+1];
				float nearDistance = intersections[ii].near;
				float farDistance = intersections[indices[endIndex-1]].far;

				if (entryCount == 1)
				{
					nearDistance = intersections[indices[0]].near;
					farDistance = intersections[indices[0]].far;
					//fragColor = vec4(0.0,1.0,1.0,1.0);
					//return;
				}

				float maximumDistance = (farDistance-nearDistance)+1.0;
				float surfaceDistance = 1.0;

				const float eps = 0.0125/10.0;
				vec4 rayOrigin = vec4(near.xyz+V*nearDistance,nearDistance);
				vec4 rayDirection = vec4(V,1.0);
				vec4 currentPosition;
				
				vec4 candidatePosition = rayOrigin;
				vec3 candidateNormal = vec3(0.0);
				vec3 candidateColor = vec3(0.0);
				float candidateValue = 0.0;

				float minimumDistance = maximumDistance;

				uint currentStep = 0;			
				float t = 0.0;

				while (++currentStep <= maximumSteps && t <= maximumDistance)
				{    
					currentPosition = rayOrigin + rayDirection*t;

//					if (currentPosition.w > closestPosition.w)
//						break;

					float sumValue = 0.0;
					vec3 sumNormal = vec3(0.0);
					vec3 sumColor = vec3(0.0);
					vec3 sumPattern = vec3(0.0);
					float sumDisplacement = 0.0;
					
					for (uint j = startIndex; j <= endIndex; j++)
					{
						uint ij = indices[j];
						uint id = intersections[ij].id;

						uint elementId = bitfieldExtract(id,0,8);
						uint residueId = bitfieldExtract(id,8,8);
						uint chainId = bitfieldExtract(id,16,8);

						vec3 aj = intersections[ij].center;
						float rj = elements[elementId].radius;
						vec3 cj = vec3(1.0,1.0,1.0);
#ifdef COLORING
						if (coloring == 1)
							cj = elements[elementId].color.rgb;
						else if (coloring == 2)
							cj = residues[residueId].color.rgb;
						else if (coloring == 3)
							cj = chains[chainId].color.rgb;
#ifdef LENSING
						cj = mix(vec3(diffuseMaterial),cj,focusFactor);
#endif

#endif
						vec3 atomOffset = currentPosition.xyz-aj;
						vec3 n0 = normalize(atomOffset);


						float delta = 0.0625;//625;//125;
						mat3 PT;
						vec2 uv;
						posToGrid(PI,n0,PT,uv);
						float frequency = 4.0;
						float amplitude = 0.0625;
						float exponent = 1.0;
						uv *= frequency;
						float displacement = amplitude*(1.0+pow(cos(uv.x)*cos(uv.y),exponent));

						vec2 uvx,uvy,uvz;
						posToGrid(PI,normalize(n0+vec3(1.0,0.0,0.0)*delta),PT,uvx);
						uvx *= frequency;
						posToGrid(PI,normalize(n0+vec3(0.0,1.0,0.0)*delta),PT,uvy);
						uvy *= frequency;
						posToGrid(PI,normalize(n0+vec3(0.0,0.0,1.0)*delta),PT,uvz);
						uvz *= frequency;

						float dpx = amplitude*(1.0+pow(cos(uvx.x)*cos(uvx.y),exponent));
						float dpy = amplitude*(1.0+pow(cos(uvy.x)*cos(uvy.y),exponent));
						float dpz = amplitude*(1.0+pow(cos(uvz.x)*cos(uvz.y),exponent));




						float fl = length(atomOffset);
						//atomOffset = atomOffset - n0*displacement;
						float atomDistance = length(atomOffset)/rj;
						
						vec3 g = 8.0*(vec3(dpx,dpy,dpz)-vec3(displacement));
						//vec3 g = vec3( -amplitude*sin(uv.x)*cos(uv.y), -amplitude*cos(uv.x)*sin(uv.y), displacement);
						vec3 gp = dot(g,n0)*n0;
						vec3 gt = g - gp;
						//n0 = n0 - gt;
						
						//sumDisplacement += (atomNormal-gt);
						//atomNormal = atomNormal+gt;

						//sumDisplacement += displacement;

						float atomValue = exp(-s*atomDistance*atomDistance);//exp(-(ad*ad)/(2.0*s*s*rj*rj)+0.5/(s*s));
						//vec3 atomNormal = 2.0*s*atomOffset*atomValue / (rj*rj);
						//vec3 atomNormal = atomValue*normalize(atomOffset);
						vec3 atomNormal = atomValue*normalize(n0);
#ifdef COLORING
						vec3 atomColor = cj*atomValue;//*mix(vec3(1.0),vec3(0.0,0.0,1.0),2.0*displacement);
#endif
						
						//float u = theta;
						//float v = phi;
						//float u = (6.0/PI)*atan(theta/sqrt(theta*theta+2.0));
						//float v = (phi*sqrt(theta*theta+2.0)) / (theta*theta+phi*phi+1.0);

						//void posToGrid(in float N, in vec3 pos, out mat3 PT, out vec2 g) {
						
						//atomNormal *= atomValue;
						//float dX = s

						//atomValue -= cos(4.0*uv.x)*cos(4.0*uv.y)*0.001;


						sumValue += atomValue;
						sumNormal += atomNormal;
#ifdef COLORING
						if (coloring > 0)
							sumColor += atomColor;
#endif
					}
					
					surfaceDistance = sqrt(-log(sumValue) / (s))-1.0;//-sumDisplacement;

					if (surfaceDistance < eps*currentPosition.w)
					{
						if (currentPosition.w <= closestPosition.w)
						{
							closestPosition = currentPosition;
							closestNormal = sumNormal;
							//closestNormal += sumDisplacement;
#ifdef COLORING
							if (coloring > 0)
							{
								diffuseColor = sumColor / sumValue;
							}


#endif
						}
						break;
					}

					if (surfaceDistance < minimumDistance)
					{
						minimumDistance = surfaceDistance;
						candidatePosition = currentPosition;
						candidateNormal = sumNormal;
						candidateColor = sumColor;
						candidateValue = sumValue;
					}

					t += surfaceDistance*1.2;
				}

				if (currentStep > maximumSteps)
				{
					if (candidatePosition.w <= closestPosition.w)
					{
						closestPosition = candidatePosition;
						closestNormal = candidateNormal;

						if (coloring > 0)
							diffuseColor = candidateColor / candidateValue;
					}
				}

				startIndex++;
			}


		}
	}
	/*
	if (closestPosition == position)
	{
		if (entryCount == 0)
		{
			fragColor = vec4(1.0,1.0,1.0,1.0);
			return;
		}
		else if (entryCount == 1)
		{
			fragColor = vec4(0.0,0.0,1.0,1.0);
		}
		else if (entryCount == 2)
		{
			fragColor = vec4(0.0,1.0,0.0,1.0);
		}
		else if (entryCount == 3)
		{
			fragColor = vec4(1.0,0.0,0.0,1.0);
		}
		else
		{
			fragColor = vec4(1.0/float(entryCount),1.0/float(entryCount),0.0,1.0);
		}
	}*/

	if (closestPosition.w >= 65535.0f)
		discard;
	//diffuseColor =+ vec3(0.25)*focusFactor;

	diffuseColor *= diffuseMaterial;

	vec4 colorSurface = shade(ambientColor,diffuseColor,specularColor,shininess,closestPosition.xyz,closestNormal.xyz,lightPosition.xyz,V.xyz);
	vec4 colorSphere = shade(ambientColor,diffuseSphereColor,specularColor,shininess,position.xyz,normal.xyz,lightPosition.xyz,V.xyz);
	//vec4 colorSphere = shade(diffuseColor,position.xyz,normal.xyz,lightPosition.xyz,V.xyz);
	//	colorSurface.a = min(1.0,0.5*length(closestPosition.xyz-position.xyz));

	/*
	vec3 cameraNormal = modelView * vec4(closestNormal.xyz,0.0);
	vec2 materialUV = 0.5*(closestNormal.xy+1.0);
	colorSurface = texture(materialTexture,materialUV);
	*/

	vec3 vU = normalize( vec3( modelView * vec4( closestPosition.xyz, 1.0 ) ) );

	vec3 n = normalize( normalMatrix * closestNormal.xyz );
	//vec3 n = normalize( vec3( modelView * vec4(closestNormal.xyz,0.0) ) );

	vec3 r = reflect( vU, n );
	float m = 2.0 * sqrt( r.x * r.x + r.y * r.y + ( r.z + 1.0 ) * ( r.z+1.0 ) );
	vec2 vN = vec2( r.x / m + 0.5,  r.y / m + 0.5 );
	
	vec4 environmentColor = vec4(1.0,1.0,1.0,1.0);
	/*
	if (environment)
	{
		vec3 N = normalize(closestNormal.xyz);
		vec3 L = normalize(lightPosition.xyz.xyz-closestPosition.xyz);
		vec3 R = normalize(reflect(L, N));

		vec2 uv = latlong(R);
		environmentColor = texture(environmentTexture,uv);
	}*/
	/*colorSurface.rgb = texture(materialTexture,vN).rgb*0.75*environmentColor.rgb;
	colorSurface.rgb += diffuseColor;
	colorSurface.a = 1.0;
	*/


	vec4 color = colorSurface;
	//color.rgb = vec3(float(entryCount)/32.0);

	//color.r -= (float(entryCount)/16.0);
	//color.a = 1.0;


	vec4 cp = modelView*vec4(closestPosition.xyz, 1.0);
	cp = cp / cp.w;
//	float dist = length(cp);

	surfacePosition = closestPosition;

	closestNormal.xyz = normalMatrix*closestNormal.xyz;
	closestNormal.xyz = normalize(closestNormal.xyz);
	surfaceNormal = vec4(closestNormal.xyz,cp.z);

	surfaceDiffuse = vec4(min(vec4(1.0),color)); 
	sphereDiffuse = colorSphere;
	gl_FragDepth = calcDepth(closestPosition.xyz);

#ifdef STATISTICS
	intersectionCount = count;
	atomicAdd(totalPixelCount,1);
	atomicAdd(totalEntryCount,entryCount);
	atomicMax(maximumEntryCount,entryCount);
#endif
}