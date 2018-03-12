#version 450

layout(pixel_center_integer) in vec4 gl_FragCoord;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat4 inverseProjection;
uniform mat4 modelViewProjection;
uniform mat4 inverseModelViewProjection;
uniform float probeRadius;

uniform vec3 lightPosition;
uniform vec3 diffuseMaterial;
uniform vec3 ambientMaterial;
uniform vec3 specularMaterial;
uniform float shininess;
uniform mat3 normalMatrix;

uniform sampler2D colorTexture;
uniform sampler2D normalTexture;
uniform sampler2D depthTexture;
uniform usampler2D offsetTexture;

in vec4 gFragmentPosition;
out vec4 fragColor;

struct BufferEntry
{
	vec4 near;
	vec4 far;
	vec3 center;
	float radius;
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
	vec4 normal = texelFetch(normalTexture,ivec2(gl_FragCoord.xy),0);
	vec4 depth = texelFetch(depthTexture,ivec2(gl_FragCoord.xy),0);

	vec4 fragCoord = gFragmentPosition;
	fragCoord /= fragCoord.w;
	
	vec4 near = inverseModelViewProjection*vec4(fragCoord.xy,-1.0,1.0);
	near /= near.w;

	vec4 far = inverseModelViewProjection*vec4(fragCoord.xy,1.0,1.0);
	far /= far.w;

	vec3 V = normalize(far.xyz-near.xyz);	
	vec3 patchColor = vec3(1.0,1.0,1.0);

	const uint maxEntries = 128;
	uint entryCount = 0;
	uint indices[maxEntries];
	
	while (offset > 0)
	{
		indices[entryCount++] = offset;
		offset = intersections[offset].previous;
	}

	if (entryCount == 0)
		discard;

	float closestDistance = position.w;

	uint startIndex = 0;
	uint endIndex = 1;

	// selection sort
	for(uint currentIndex = 0; currentIndex <= entryCount/* - 1*/; currentIndex++)
	{
		uint minimumIndex = currentIndex;

		for(uint i = currentIndex+1; i < entryCount; i++)
		{
			if(intersections[indices[i]].near.w < intersections[indices[minimumIndex]].near.w)
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

//#define POSTSORT

#ifndef POSTSORT
		if (currentIndex > 0)
		{
			if (currentIndex < entryCount && intersections[indices[startIndex]].far.w >= intersections[indices[currentIndex]].near.w)
			{
				endIndex = currentIndex;
			}
			else
			{
				if (currentIndex >= entryCount)
					endIndex = entryCount-1;

				const uint maximumSteps = 64;
				const float maximumDistance = 16.0;
				const float s = 2.5;

				uint ii = indices[startIndex];
				vec4 nearPosition = intersections[ii].near;
				vec4 farPosition = intersections[indices[endIndex]].far;

				vec4 rayOrigin = nearPosition;
				vec4 rayDirection = vec4(V,1.0);
				vec4 currentPosition;
					
				float t = 0.0;
				float h = 1.0;

				uint currentStep = 0;

				while (++currentStep <= maximumSteps && t < maximumDistance)
				{    
					currentPosition = rayOrigin + rayDirection*t;

					if (currentPosition.w >= closestDistance)
						break;

					float sg = 0.0;
					float sr = 0.0;
					vec3 sn = vec3(0.0);
					
					for (uint j = startIndex; j <= endIndex; j++)
					{
						uint ij = indices[j];
						vec3 aj = intersections[ij].center;
						float rj = intersections[ij].radius; 

						float ad = length(currentPosition.xyz-aj)-rj;
						float gi = exp(-s*ad);
						vec3 ni = -(currentPosition.xyz-aj)*gi;

						sg += gi;
						sn += ni;
					}

					sg = -log(sg) / s;
					h = sg;

					if ( h < 0.01)
					{
						if (currentPosition.w < closestDistance)
						{
							normal.xyz = -normalize(sn);
							closestDistance = currentPosition.w;
							///patchColor = vec3(1.0,0.0,1.0);
						}
						break;
					}


					t += h;
				}

				startIndex++;
			}
		}

#endif

	}

#ifdef POSTSORT
	if (entryCount > 0)
	{
		for (uint i=0;i < entryCount;i++)
		{		
			vec3 nn = vec3(1000.0,1000.0,1000.0);
			const float s = 3.75;
			float th = 1.0;
			float d;
			vec3 norm = vec3(0.0,0.0,0.0);

			vec3 x;
			float maxd = 64.0;
			float h = 1.0;
			float t = 0.0;

			vec4 nearPosition = intersections[indices[i]].near;
			vec4 farPosition = intersections[indices[i]].far;
			vec3 ro = nearPosition.xyz;
			vec3 rd = V;
			uint j;

			for( int steps=0; steps<256; steps++ )
			{
				if( h<0.001 || t>maxd )
					break;
	    
				x = ro+rd*t;
				
				j = i;

				float sg = 0.0;
				float sr = 0.0;
				vec3 sn = vec3(0.0);

				while (j < entryCount && intersections[indices[i]].far.w >= intersections[indices[j]].near.w)
				{
					uint ij = indices[j];
					vec3 aj = intersections[ij].center;
					float rj = intersections[ij].radius; 

					float ad = length(x-aj)-rj;
					float gi = exp(-s*ad);
					vec3 ni = -(x-aj)*gi;

					sg += gi;
					sn += ni;

					j++;

				}

				sg = -log(sg) / s;
				nn = sn;

				h = sg;
				t += h;

			}

			if( h<0.001 && t<maxd )
			{
				float dd = length(x-near.xyz);

				if (dd < depth.r)
				{
					normal.xyz = -normalize(nn);
					//patchColor = vec3(1.0,0.0,1.0);
					break;
				}
			}
		}
	}
#endif

	if (closestDistance >= 65535.0f)
		discard;

		// vectors
	vec3 N = normalize(-normal.xyz);//intersections[indices[0]].center.xyz - intersections[indices[0]].near.xyz);
	vec3 L = lightPosition;
	vec3 R = normalize(reflect(L, N));
	float NdotL = abs(dot(N, L));
	float RdotV = abs(dot(R, -V));

	vec3 c = ambientMaterial + NdotL * diffuseMaterial + pow(RdotV,shininess) * specularMaterial;
	//vec3 c = color;// color.rgb*vec3(63.0/255.0,136/255.0,189/255.0);// + 0.125*color.rgb*pow(RdotV,shininess) * specularMaterial;//0.5*color.rgb + 0.5*color.rgb*(0.75+0.25 * NdotL) * diffuseMaterial + color.rgb*pow(RdotV,shininess) * specularMaterial;

	fragColor = vec4(min(vec3(1.0),c.xyz),1.0);//vec4(offset,1.0,0.0,1.0);
//	fragColor.rgb *= patchColor;
		
/*		
	if (entryCount == 0)
		fragColor = vec4(0.25,0.25,0.25,1.0);
	else if (entryCount == 1)
		fragColor = mix(fragColor,vec4(0.0,0.0,1.0,1.0),0.5);
	else if (entryCount == 2)
		fragColor = mix(fragColor,vec4(0.0,1.0,0.0,1.0),0.5);
	else
		fragColor = mix(fragColor,vec4(1.0-float(entryCount)/4.0,0.0,0.0,1.0),0.5);
*/		
/*		
	if (entryCount > 0)
	{
	float dis = length(pos.xyz-intersections[indices[0]].near.xyz);
	fragColor = mix(fragColor,vec4(float(entryCount)/10.0,0.0,0.0,1.0),0.5);
	}
*/
	fragColor.a = 1.0;
		
	//fragColor = vec4(0.5+float(entryCount)/10.0,0.0,0.0,1.0);

	//fragColor.xyz = intersections[indices[0]].center.xyz;
	gl_FragDepth = depth.z;

}