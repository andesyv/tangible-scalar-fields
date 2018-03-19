#version 450

layout(pixel_center_integer) in vec4 gl_FragCoord;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat4 inverseProjection;
uniform mat4 modelViewProjection;
uniform mat4 inverseModelViewProjection;
uniform float probeRadius;
uniform float softness;
uniform bool coloring;

uniform vec3 lightPosition;
uniform vec3 diffuseMaterial;
uniform vec3 ambientMaterial;
uniform vec3 specularMaterial;
uniform float shininess;

uniform sampler2D colorTexture;
uniform sampler2D normalTexture;
uniform sampler2D depthTexture;
uniform usampler2D offsetTexture;

in vec4 gFragmentPosition;
out vec4 fragColor;
out vec4 fragNormal;

struct AtomData
{
	vec3 color;
	float radius;
};

layout(std140, binding = 0) uniform atomBlock
{
	AtomData atoms[32];
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


void main()
{
	uint offset = texelFetch(offsetTexture,ivec2(gl_FragCoord.xy),0).r;

	if (offset == 0)
		discard;

	vec4 position = texelFetch(colorTexture,ivec2(gl_FragCoord.xy),0);
	vec3 normal = texelFetch(normalTexture,ivec2(gl_FragCoord.xy),0).xyz;
	//vec4 depth = texelFetch(depthTexture,ivec2(gl_FragCoord.xy),0);

	vec4 fragCoord = gFragmentPosition;
	fragCoord /= fragCoord.w;
	
	vec4 near = inverseModelViewProjection*vec4(fragCoord.xy,-1.0,1.0);
	near /= near.w;

	vec4 far = inverseModelViewProjection*vec4(fragCoord.xy,1.0,1.0);
	far /= far.w;

	vec3 V = normalize(far.xyz-near.xyz);	
	float curvature = 1.0;

	const uint maxEntries = 128;
	uint entryCount = 0;
	uint indices[maxEntries];

	vec3 diffuseColor = vec3(1.0,1.0,1.0);
	
	while (offset > 0)
	{
		indices[entryCount++] = offset;
		offset = intersections[offset].previous;
	}

	if (entryCount == 0)
		discard;

	vec4 closestPosition = position;
	vec3 closestNormal = normal;

	uint startIndex = 0;
	uint endIndex = 1;

	// selection sort
	for(uint currentIndex = 0; currentIndex <= entryCount/* - 1*/; currentIndex++)
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

		if (currentIndex > 0)
		{
			if (currentIndex < entryCount && intersections[indices[startIndex]].far >= intersections[indices[currentIndex]].near)
			{
				endIndex = currentIndex;
			}
			else
			{
				if (currentIndex >= entryCount)
					endIndex = entryCount-1;

				const uint maximumSteps = 128;
				const float s = softness;
				const float eps = 0.001;

				uint ii = indices[startIndex];
				vec3 ai = intersections[ii].center;
				float ri = atoms[intersections[ii].id].radius;
				float nearDistance = intersections[ii].near;
				float farDistance = intersections[indices[endIndex]].far;

				const float maximumDistance = 16.0;//farDistance-nearDistance;

				float surfaceDistance = 1.0;

				vec4 rayOrigin = vec4(near.xyz+V*nearDistance,nearDistance);
				vec4 rayDirection = vec4(V,1.0);
				vec4 currentPosition;
				

				uint currentStep = 0;			
				float t = 0.0;

				while (++currentStep <= maximumSteps && t < maximumDistance)
				{    
					currentPosition = rayOrigin + rayDirection*t;

					if (currentPosition.w > closestPosition.w)
						break;

					float sumValue = 0.0;
					vec3 sumNormal = vec3(0.0);
					vec3 sumColor = vec3(0.0);
					
					for (uint j = startIndex; j <= endIndex; j++)
					{
						uint ij = indices[j];

						vec3 aj = intersections[ij].center;
						float rj = atoms[intersections[ij].id].radius;
						vec3 cj = atoms[intersections[ij].id].color;

						vec3 atomOffset = currentPosition.xyz-aj;						
						float atomDistance = length(atomOffset);

						float atomValue = exp(-s*atomDistance*atomDistance/(rj*rj)+s/(rj*rj));//exp(-(ad*ad)/(2.0*s*s*rj*rj)+0.5/(s*s));
						vec3 atomNormal = normalize(atomOffset)*atomValue;
						vec3 atomColor = cj*atomValue;

						sumValue += atomValue;
						sumNormal += atomNormal;

						if (coloring)
							sumColor += atomColor;
					}
					
					surfaceDistance = sqrt(-log(sumValue) / (s))-1.0;

					if (surfaceDistance < eps)
					{
						if (currentPosition.w <= closestPosition.w)
						{
							closestPosition = currentPosition;
							closestNormal = normalize(sumNormal);

							if (coloring)
								diffuseColor = sumColor / sumValue;
						}
						break;
					}


					t += surfaceDistance;
				}

				startIndex++;
			}
		}
	}


	if (closestPosition.w >= 65535.0f)
		discard;

		// vectors
	vec3 N = normalize(closestNormal);
	vec3 L = normalize(lightPosition.xyz-closestPosition.xyz);
	vec3 R = normalize(reflect(L, N));
	float NdotL = max(0.0,dot(N, L));
	float RdotV = max(0.0,dot(R, V));
	vec3 color = ambientMaterial + NdotL * diffuseMaterial * diffuseColor + pow(RdotV,shininess) * specularMaterial;
	/*
	vec3 N1 = normalize(normal);
	vec3 L1 = normalize(lightPosition.xyz-position.xyz);
	vec3 R1 = normalize(reflect(L1, N1));
	float NdotL1 = max(0.0,dot(N1, L1));
	float RdotV1 = max(0.0,dot(R1, V));
	vec3 color1 = ambientMaterial + NdotL1 * diffuseMaterial * diffuseColor + pow(RdotV1,shininess) * specularMaterial;
	color = 0.5*color+0.5*color1;
	*/

	
	float contour = 1.0;
	float c = abs(dot(N,V));
	float t = fwidth(c);

	//color /= (1.0-pow(1.0-c,4.0));
		

	fragColor = vec4(min(vec3(1.0),color.xyz),1.0);
	fragNormal = vec4(N,0.0);
	gl_FragDepth = calcDepth(closestPosition.xyz);
}