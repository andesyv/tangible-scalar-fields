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
	vec4 fragCoord = gFragmentPosition;
	fragCoord /= fragCoord.w;
	
	vec4 near = inverseModelViewProjection*vec4(fragCoord.xy,-1.0,1.0);
	near /= near.w;

	vec4 far = inverseModelViewProjection*vec4(fragCoord.xy,1.0,1.0);
	far /= far.w;

	vec3 V = normalize(far.xyz-near.xyz);	

	vec4 position = texelFetch(colorTexture,ivec2(gl_FragCoord.xy),0);
	vec4 normal = texelFetch(normalTexture,ivec2(gl_FragCoord.xy),0);
	vec4 depth = texelFetch(depthTexture,ivec2(gl_FragCoord.xy),0);
	uint offset = texelFetch(offsetTexture,ivec2(gl_FragCoord.xy),0).r;
	depth.r = position.w;

	vec3 patchColor = vec3(1.0,1.0,1.0);

/*
	fragColor = vec4(offset,0.0,0.0,1.0);
	if (position.w < 1.0)
		fragColor = vec4(0.0,0.0,1.0,0.0);
	return;*/
	vec4 minimumNear = vec4(0.0,0.0,0.0,1.0);

	const uint maxEntries = 128;
	uint entryCount = 0;
	uint indices[maxEntries];

/*
	// minResolve
	while (offset > 0)
	{
		BufferEntry entry = intersections[offset];

		if (entry.near.w < minimumNear.w)
		{
			minimumNear = entry.near;
		}

		offset = entry.previous;
	}
*/
	while (offset > 0)
	{
		indices[entryCount++] = offset;
		offset = intersections[offset].previous;
	}

	int overlaps = 0;
	vec4 pos = position;
	
	if (entryCount > 0)
	{
		uint startIndex = 0;
		uint endIndex = 0;

		// selection sort
		for(uint j = 0; j <= entryCount/* - 1*/; j++)
		{
			uint minimumIndex = j;

			for(uint i = j+1; i < entryCount; i++)
			{
				if(intersections[indices[i]].near.w < intersections[indices[minimumIndex]].near.w)
				{
					minimumIndex = i;					
				}
			}

			if (minimumIndex != j)
			{
				uint temp = indices[minimumIndex];
				indices[minimumIndex] = indices[j];
				indices[j] = temp;
			}

			if (j > 0)
			{
				if (j >= entryCount || intersections[indices[j-1]].far.w < intersections[indices[j]].near.w)
				{
					endIndex = j-1;

					if (startIndex < endIndex)
					{
						vec3 x;
						float maxd = 32.0;
						float t = 0.0;
						float s = 1.0;

						vec4 nearPosition = intersections[indices[startIndex]].near;
						vec4 farPosition = intersections[indices[endIndex]].far;
						vec3 ro = nearPosition.xyz;

						uint steps = 0;
						uint maxSteps = 64;
						vec3 rd = (farPosition.xyz-nearPosition.xyz) / maxSteps;

						float h = 1.0;

						uint ii = indices[startIndex];
						vec3 ai = intersections[ii].center;
						float ri = intersections[ii].radius; 


						while (++steps < maxSteps)
						{    
							x = ro+rd*t;

							if (length(x-near.xyz) >= position.w)
								break;

							float sg = 0.0;
							float sr = 0.0;
							vec3 sn = vec3(0.0);
							float minD = 100000.0;
					
							for (uint i = startIndex; i <= endIndex; i++)
							{
								uint ij = indices[i];
								vec3 aj = intersections[ij].center;
								float rj = intersections[ij].radius; 
								
								float rp = probeRadius;
								float dij = length(aj-ai);
								vec3 uij = (aj-ai)/dij;
								vec3 tij = 0.5*(ai+aj)+0.5*(aj-ai)*((ri+rp)*(ri+rp) - (rj+rp)*(rj+rp))/(dij*dij);
								float rij_a = (ri+rj+2.0*rp)*(ri+rj+2.0*rp)-dij*dij;
								float rij_b = dij*dij-(ri-rj)*(ri-rj);

								float rij = 0.5 * sqrt(rij_a) * sqrt(rij_b) / dij;
								float rc = rij*ri / (ri+rp);

								vec3 c = tij;
								vec2 r = vec2(rij,rp);
								mat3 m = rotationAlign(vec3(0.0,1.0,0.0),uij);

								vec3 tx = (x-c)*m;
								float td = blendDistance(tx,r);
								vec3 pp = x-tij;
								vec3 xx = rij*normalize(pp-dot(uij,pp)*uij);

								
/*
								float gi = gauss(x,ai,ri,s);
								vec3 ni = -(x-ai)*gi/(ri*ri+s*s);
*/
								float ad = length(x-aj);
								float gi = gauss(ad,rj,s);
								vec3 ni = -(x-aj)*gi/(rj*rj+s*s);

								sg += gi;
								sn += ni;
								/*
								float R = ri*2.0;
								float r = length(x-ai);

								if (r < R)
								{
									float d = blobby(r,R);
									vec3 ni = (x-ai)*d;

									sg += d;
									sn += ni*((6.0*r*r)/(R*R*R)-6.0*(r/(R*R)));
									sr += R;
								}

								minD = min(minD,r-R);
								*/
							}
							/*
							float thres = 0.0;
							sg = max(minD,(2.0/3.0)*(thres-sg)*sr);
							h = sg;

							if (h > maxd)
								break;
							*/
							if ( sg >= 1.0)
							{
								float dd = length(x-near.xyz);
								normal.xyz = -normalize(sn);
								depth.r = dd;
								pos = vec4(x,dd);
								patchColor = vec3(1.0,1.0,1.0);
								break;
							}

							h = 1.0;

							t += (h);
						}
					}

					startIndex = j;

				}
			}
		}







//////////////////////////////////////
//////////////////////////////////////
//////////////////////////////////////
// NEW IMPLICIT ATTEMPT (DESPERATION!) - start
#ifdef WHATEV
		const float eps = 0.001;
		for (uint i=0;i < entryCount-1;i++)
		{
			if (depth.r < intersections[indices[i]].near.w)
				break;
			
			vec3 nn = vec3(1000.0,1000.0,1000.0);
			const float s = 1.0;
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

			for( int steps=0; steps<256; steps++ )
			{
				if( h<0.001 || t>maxd )
					break;
	    
				x = ro+rd*t;
				
				d= 1000.0;
				uint j = i+1;

				while (j < entryCount && intersections[indices[i]].far.w >= intersections[indices[j]].near.w)
				{
					uint ii = indices[i];
					uint ij = indices[j];

					sort2(intersections[ii].id,ii,intersections[ij].id,ij);
					vec3 ai = intersections[ii].center;
					vec3 aj = intersections[ij].center;
					float ri = intersections[ii].radius; 
					float rj = intersections[ij].radius;
					float rp = probeRadius;
					float dij = length(aj-ai);
					vec3 uij = (aj-ai)/dij;
					vec3 tij = 0.5*(ai+aj)+0.5*(aj-ai)*((ri+rp)*(ri+rp) - (rj+rp)*(rj+rp))/(dij*dij);
					float rij_a = (ri+rj+2.0*rp)*(ri+rj+2.0*rp)-dij*dij;
					float rij_b = dij*dij-(ri-rj)*(ri-rj);

					if (rij_a > eps && rij_b > eps)
					{
						float rij = 0.5 * sqrt(rij_a) * sqrt(rij_b) / dij;
						float rc = rij*ri / (ri+rp);

						vec3 c = tij;
						vec2 r = vec2(rij,rp);
						mat3 m = rotationAlign(vec3(0.0,1.0,0.0),uij);

						vec3 tx = (x-c)*m;
						float td = blendDistance(tx,r);


						vec3 pp = x-tij;
						vec3 xx = rij*normalize(pp-dot(uij,pp)*uij);
						
						d = min(d,td);
						nn = min(nn,pp-xx);
					}

					j++;

				}

				h = d;
				t += h;

			}

			if( h<0.001 && t<maxd )
			{
				float dd = length(x-near.xyz);

				if (dd < depth.r)
				{
					normal.xyz = -normalize(nn);
					depth.r = dd;
//					patchColor = vec3(1.0,0.0,0.0);
					pos = vec4(x,dd);
					break;
				}
			}

		}
#endif
// NEW IMPLICIT ATTEMPT (DESPERATION!) - end
//////////////////////////////////////
//////////////////////////////////////
//////////////////////////////////////















// ALL THIS IS COMMENTED OUT
#ifdef SEPP

		ivec3 sid = ivec3(0,0,0);

		const float eps = 0.001;
		for (uint i=0;i < entryCount-1;i++)
		{
			if (depth.r < intersections[indices[i]].near.w)
				break;
			
			uint j = i+1;
			uint count = 0;
			bool hit = false;

			while (j < entryCount && intersections[indices[i]].far.w >= intersections[indices[j]].near.w)
			{
				if (depth.r < intersections[indices[i]].near.w)
					break;
				//if (false)
				{
					uint ii = indices[i];
					uint ij = indices[j];

					// Torus
					vec4 nearPosition = intersections[ii].near;
					vec4 farPosition = intersections[ii].far;

					sort2(intersections[ii].id,ii,intersections[ij].id,ij);
					vec3 ai = intersections[ii].center;
					vec3 aj = intersections[ij].center;
					float ri = intersections[ii].radius; 
					float rj = intersections[ij].radius;
					float rp = probeRadius;
					float dij = length(aj-ai);
					vec3 uij = (aj-ai)/dij;
					vec3 tij = 0.5*(ai+aj)+0.5*(aj-ai)*((ri+rp)*(ri+rp) - (rj+rp)*(rj+rp))/(dij*dij);
					float rij_a = (ri+rj+2.0*rp)*(ri+rj+2.0*rp)-dij*dij;
					float rij_b = dij*dij-(ri-rj)*(ri-rj);

					if (rij_a > eps && rij_b > eps)
					{
						float rij = 0.5 * sqrt(rij_a) * sqrt(rij_b) / dij;
						float rc = rij*ri / (ri+rp);

						vec3 x;
						if (intersectTorus( nearPosition.xyz, V.xyz, x, tij, uij, vec2(rij,rp) ))				
						{
							float dd = length(x-near.xyz);//calcDepth(x);

							if (dd < depth.r)
							{
								vec3 pp = x-tij;
								vec3 xx = rij*normalize(pp-dot(uij,pp)*uij);
								vec3 nn = pp-xx;
						
								normal.xyz = -normalize(nn);
								depth.r = dd;
								patchColor = vec3(0.0,1.0,0.0);
								pos = vec4(x,dd);
								hit = true;

							}
			//				else
				//				break;
						}
					}
				}


				uint k = j+1;

				uvec3 probeIndices[16];
				vec3 probesNear[16];
				vec3 probesFar[16];
				vec3 torusRadii[16];
				vec3 probeBase[16];
				vec3 baseNormal[16];
				uint probeCount = 0;

				while (false && k < entryCount && intersections[indices[j]].far.w >= intersections[indices[k]].near.w && intersections[indices[j]].far.w >= intersections[indices[k]].near.w && intersections[indices[j]].far.w >= intersections[indices[k]].near.w)
				{
					uint ii = indices[i];
					uint ij = indices[j];
					uint ik = indices[k];					
					
					vec4 nearPosition = intersections[ii].near;
					vec4 farPosition = intersections[ii].far;
					//fragColor = vec4(1.0,0.0,0.0,1.0);
					//return;

					sort3(intersections[ii].id,ii,intersections[ij].id,ij,intersections[ik].id,ik);
					//swap(ii,ik);

					vec3 ai = intersections[ii].center;
					vec3 aj = intersections[ij].center;
					vec3 ak = intersections[ik].center;

					float ri = intersections[ii].radius; 
					float rj = intersections[ij].radius;
					float rk = intersections[ik].radius;
					float rp = probeRadius;

					float dij = length(aj-ai);
					float dik = length(ak-ai);
					float djk = length(ak-aj);

					vec3 uij = (aj-ai)/dij;
					vec3 uik = (ak-ai)/dik;
					vec3 ujk = (aj-ak)/djk;

					vec3 tij = 0.5*(ai+aj)+0.5*(aj-ai)*((ri+rp)*(ri+rp) - (rj+rp)*(rj+rp))/(dij*dij);
					vec3 tik = 0.5*(ai+ak)+0.5*(ak-ai)*((ri+rp)*(ri+rp) - (rk+rp)*(rk+rp))/(dik*dik);
					vec3 tjk = 0.5*(aj+ak)+0.5*(ak-aj)*((rj+rp)*(rj+rp) - (rk+rp)*(rk+rp))/(djk*djk);


					float rij = 0.0;
					float rij_a = (ri+rj+2.0*rp)*(ri+rj+2.0*rp)-dij*dij;
					float rij_b = dij*dij-(ri-rj)*(ri-rj);

					if (rij_a > 0.0 && rij_b > 0.0)
						rij = 0.5 * sqrt(rij_a) * sqrt(rij_b) / dij;

					float rik = 0.0;
					float rik_a = (ri+rk+2.0*rp)*(ri+rk+2.0*rp)-dik*dik;
					float rik_b = dik*dik-(ri-rk)*(ri-rk);

					if (rik_a > 0.0 && rik_b > 0.0)
						rik = 0.5 * sqrt(rik_a) * sqrt(rik_b) / dik;

					float rjk = 0.0;
					float rjk_a = (rj+rk+2.0*rp)*(rj+rk+2.0*rp)-djk*djk;
					float rjk_b = djk*djk-(rj-rk)*(rj-rk);

					if (rjk_a > 0.0 && rjk_b > 0.0)
						rjk = 0.5 * sqrt(rjk_a) * sqrt(rjk_b) / djk;

					vec3 uijk = normalize(cross(uij,uik));
					vec3 utb = cross(uijk,uij);
					vec3 bijk = tij + utb*(dot(uik,tik-tij)) / dot(utb,uik);

					float hijk_a = (ri+rp)*(ri+rp) - length(bijk-ai)*length(bijk-ai);

					if (hijk_a > 0.0 && rij > 0.0 && rik > 0.0 && rjk > 0.0)
					{
						//fragColor = vec4(1.0,1.0,0.0,1.0);
						//return;

						float hijk = sqrt(hijk_a);

						if (hijk > rp)
						{
							vec3 pijk0 = bijk - hijk*uijk;						
							vec3 pijk1 = bijk + hijk*uijk;

					//		if (length(pijk0-ai) >= ri+probeRadius-eps && length(pijk0-aj) >= rj+probeRadius-eps && length(pijk0-ak) >= rk+probeRadius-eps &&
					//			length(pijk1-ai) >= ri+probeRadius-eps && length(pijk1-aj) >= rj+probeRadius-eps && length(pijk1-ak) >= rk+probeRadius-eps)
							{
								probesNear[probeCount] = pijk0;
								probesFar[probeCount] = pijk1;

								probeIndices[probeCount] = uvec3(ii,ij,ik);
								torusRadii[probeCount] = vec3(rij,rik,rjk);
								probeBase[probeCount] = bijk;
								baseNormal[probeCount] = uijk;

								probeCount++;
							}
						}
					}

					k+=1;
				}

				vec3 farHit;
				vec3 farNormal;
				float maxFar = 0.0;
				bool bHit = false;

				for (k=0;k<probeCount;k++)
				{
					uint ii = probeIndices[k].x;
					uint ij = probeIndices[k].y;
					uint ik = probeIndices[k].z;					

					vec3 ai = intersections[ii].center;
					vec3 aj = intersections[ij].center;
					vec3 ak = intersections[ik].center;

					vec3 pijk0 = probesNear[k];
					vec3 pijk1 = probesFar[k];

					float rij = torusRadii[k].x;
					float rik = torusRadii[k].y;
					float rjk = torusRadii[k].z;

					vec3 bijk = probeBase[k];
					vec3 uijk = baseNormal[k];

					Sphere s0 = calcSphereIntersection(probeRadius,near.xyz,pijk0,V);
					Sphere s1 = calcSphereIntersection(probeRadius,near.xyz,pijk1,V);

					float ddn0 = length(s0.near.xyz-near.xyz);
					float ddf0 = length(s0.far.xyz-near.xyz);

					float ddn1 = length(s1.near.xyz-near.xyz);
					float ddf1 = length(s1.far.xyz-near.xyz);

					if (s0.hit && s1.hit)
					{
						if (ddf0 < ddf1)
							s1.hit = false;
						else
							s0.hit = false;
					}

					if (s0.hit && ddn0 > maxFar)
					{
						vec3 n1 = normalize(cross(ak-ai,pijk0-ai));
						vec3 n2 = normalize(cross(ak-aj,pijk0-aj));
						vec3 n3 = normalize(cross(aj-ai,pijk0-ai));
						
						bool bClipped = false;
						
						if (dot(s0.far.xyz-ai,n1) > 0.0)
							bClipped = true;
						else if (dot(s0.far.xyz-aj,n2) < 0.0)
							bClipped = true;
						else if (dot(s0.far.xyz-ai,n3) < 0.0)
							bClipped = true;

						if (!bClipped)
						{
							maxFar = ddf0;
							farHit = s0.far.xyz;
							farNormal = normalize(s0.far.xyz-pijk0);
							bHit = true;
						}
					}

					/*
					if (rij < probeRadius)
					{
						patchColor = vec3(0.5,1.0,1.0);
					}

					if (rik < probeRadius)
					{
						patchColor = vec3(1.0,0.5,1.0);
					}

					if (rjk < probeRadius)
					{
						patchColor = vec3(1.0,1.0,0.0);
					}*/
					/*
					if (s0.hit && s1.hit)
					{
						if (ddf0> ddn1)
						{
							float denom = dot(V,uijk);

							if (abs(denom) > eps)
							{
								float t = dot(bijk-near.xyz,uijk) / denom;
								s0.far.xyz = near.xyz + t*V.xyz;
							}

						}
					}*/

					if (s0.hit)
					{
						float dd = length(s0.far.xyz-near.xyz);//calcDepth(s.far.xyz);
						bool bClipped = false;

						/*
						if (length(s0.far.xyz-pijk1) < probeRadius)
						{
							if (dot(s0.far.xyz-(aj+ak)*0.5,cross(V,normalize(ak-aj))) < 0.0)
								bClipped = true;

							if (dot(s0.far.xyz-(aj+ak)*0.5,V) < 0.0)
								bClipped = true;
						}
						*/

						vec3 n1 = normalize(cross(ak-ai,pijk0-ai));
						vec3 n2 = normalize(cross(ak-aj,pijk0-aj));
						vec3 n3 = normalize(cross(aj-ai,pijk0-ai));
						
						
						if (dot(s0.far.xyz-ai,n1) > 0.0)
							bClipped = true;
						else if (dot(s0.far.xyz-aj,n2) < 0.0)
							bClipped = true;
						else if (dot(s0.far.xyz-ai,n3) < 0.0)
							bClipped = true;

//						bClipped = false;

						//if (length(s0.far.xyz-pijk1) < probeRadius)
							//bClipped = true;

						if (s1.hit && ddf0 > ddn1 && ddf1 > ddn0)
						{
						//	s0.far = s1.far;
						//	if (dot(s0.far.xyz-bijk,uijk) < 0.0)
						//		bClipped = true;
						}


						//if (s1.hit && ddf0 > ddn1 && ddf1 > ddn0)

						//if (rij < probeRadius || rik < probeRadius || rjk < probeRadius)
						if (false)
						{
							for (uint l=0;l<probeCount;l++)
							{
								if (l != k)
								{
									if (length(s0.far.xyz-probesNear[l]) < probeRadius-eps)
									{
										bClipped = true;
										break;
									}
							/*		else if (length(s0.far.xyz-probesFar[l]) < probeRadius-eps)
									{
										bClipped = true;
										break;
									}*/
								}
							}

							//bClipped = clipThis;
							//patchColor = vec3(1.0,1.0,1.0);
						}
						/*
						if (s1.hit)
						{
							if (ddf0> ddn1)
							{
								bClipped = true;
							}
						}*/


						if (!bClipped)
						{
							if (dd <= depth.r)
							{
								depth.r = dd;
								normal.xyz = normalize(s0.far.xyz-pijk0);// s.near.cenm-normalize(s.far.xyz-sp);
								patchColor = vec3(1.0,1.0,0.0);
								sid = ivec3(i,j,k);
								pos = vec4(s0.far.xyz,dd);
								hit = true;

								//if (s1.hit && ddf0 > ddn1 && ddf1 > ddn0)
								{
								}
							}
						//	else
						//		break;

						//	break;
						}
					}
						
					if (s1.hit)
					{
						float dd = length(s1.far.xyz-near.xyz);//calcDepth(s.far.xyz);
						bool bClipped = false;

						if (length(s1.far.xyz-pijk0) < probeRadius)
						{
						/*
							if (dot(s1.far.xyz-(ai+aj)*0.5,cross(V,normalize(aj-ai))) > 0.0)
								bClipped = true;

							if (dot(s1.far.xyz-(ai+aj)*0.5,V) < 0.0)
								bClipped = true;
						*/
						}


						vec3 n1 = normalize(cross(ak-ai,pijk1-ai));
						vec3 n2 = normalize(cross(ak-aj,pijk1-aj));
						vec3 n3 = normalize(cross(aj-ai,pijk1-ai));
																	
						if (dot(s1.far.xyz-ai,n1) < 0.0)
							bClipped = true;
						else if (dot(s1.far.xyz-aj,n2) > 0.0)
							bClipped = true;
						else if (dot(s1.far.xyz-ai,n3) > 0.0)
							bClipped = true;

						//if (dot(s1.far.xyz-bijk,uijk) < 0.0)
							//bClipped = true;


//						bClipped = false;
						
						if (s0.hit && ddf0 > ddn1 && ddf1 > ddn0)
						{
							s1.far = s0.far;

							//if (ivec3(i,j,k) != sid && sid != ivec3(0,0,0))
							//	bClipped = true;
						}

/*
						if (s0.hit)
						{
							if (ddf1> ddn0)
							{
						//		s1.far = s0.far;
						//		bClipped = true;
							}
						}
*/

//						if (rij < probeRadius || rik < probeRadius || rjk < probeRadius)
 						if (false)
						{
							for (uint l=0;l<probeCount;l++)
							{
								//if (l != k)
								{
									if (length(s1.far.xyz-probesNear[l]) < probeRadius-eps)
									{
										bClipped = true;
										break;
									}
									else if (length(s1.far.xyz-probesFar[l]) < probeRadius-eps)
									{
										bClipped = true;
										break;
									}
								}
							}
						}

						if (!bClipped)
						{
							if (dd <= depth.r)
							{
								depth.r = dd;
								normal.xyz = normalize(s1.far.xyz-pijk1);// s.near.cenm-normalize(s.far.xyz-sp);
								patchColor = vec3(1.0,0.0,0.0);
								sid = ivec3(i,j,k);
								pos = vec4(s1.far.xyz,dd);
								hit = true;

								//if (s0.hit && ddf0 > ddn1 && ddf1 > ddn0)
								{
									if (rij < probeRadius || rik < probeRadius || rjk < probeRadius)
										patchColor = vec3(1.0,0.0,1.0);
								}
							}
					//		else
								//break;
						//	break;
						}
					}
				}

/*				if (bHit)
				{
					if (maxFar <= depth.r)
					{
						normal.xyz = farNormal;
						depth.r = maxFar;
					}
				}
*/



				j++;
				count++;
			}

/*
			if (j < entryCount-2 &&
				intersections[indices[j+1]].near.w >= intersections[indices[j]].near.w && intersections[indices[j+1]].near.w <= intersections[indices[j]].far.w &&
				intersections[indices[j+2]].near.w >= intersections[indices[j]].near.w && intersections[indices[j+2]].near.w <= intersections[indices[j+1]].far.w &&
				intersections[indices[j+2]].near.w >= intersections[indices[j+1]].near.w && intersections[indices[j+2]].near.w <= intersections[indices[j+1]].far.w)
*/
/*			if (j < entryCount-2 && intersections[indices[j]].far.w >= intersections[indices[j+1]].near.w && intersections[indices[j]].far.w >= intersections[indices[j+2]].near.w)
				//intersections[indices[j+1]].far.w >= intersections[indices[j+2]].near.w)
			{
				uint ii = indices[j];
				uint ij = indices[j+1];
				uint ik = indices[j+2];					
					
				vec4 nearPosition = intersections[ii].near;
				vec4 farPosition = intersections[ii].far;
				//fragColor = vec4(1.0,0.0,0.0,1.0);
				//return;

				sort3(intersections[ii].id,ii,intersections[ij].id,ij,intersections[ik].id,ik);
				//swap(ii,ik);

				vec3 ai = intersections[ii].center;
				vec3 aj = intersections[ij].center;
				vec3 ak = intersections[ik].center;

				float ri = intersections[ii].radius; 
				float rj = intersections[ij].radius;
				float rk = intersections[ik].radius;
				float rp = probeRadius;

				float dij = length(aj-ai);
				float dik = length(ak-ai);
				float djk = length(ak-aj);

				vec3 uij = (aj-ai)/dij;
				vec3 uik = (ak-ai)/dik;
				vec3 ujk = (aj-ak)/djk;

				vec3 tij = 0.5*(ai+aj)+0.5*(aj-ai)*((ri+rp)*(ri+rp) - (rj+rp)*(rj+rp))/(dij*dij);
				vec3 tik = 0.5*(ai+ak)+0.5*(ak-ai)*((ri+rp)*(ri+rp) - (rk+rp)*(rk+rp))/(dik*dik);
				vec3 tjk = 0.5*(aj+ak)+0.5*(ak-aj)*((rj+rp)*(rj+rp) - (rk+rp)*(rk+rp))/(djk*djk);

				float rij = 0.5*sqrt((ri+rj+2.0*rp)*(ri+rj+2.0*rp)-dij*dij) * sqrt(dij*dij-(ri-rj)*(ri-rj)) / dij;

				//float wijk = acos(dot(uij,uik));
				//vec3 uijk = cross(uij,uik)/sin(wijk);

				vec3 uijk = normalize(cross(uij,uik));
				//vec3 uijk = normalize(cross(uij,ujk));

				vec3 utb = cross(uijk,uij);
				vec3 bijk = tij + utb*(dot(uik,tik-tij)) / dot(utb,uik);

				//float cs = 

				//vec3 bijk = ai*ci + aj*cj + ak*ck;

				//sin(wijk);//(tij+tik+tjk)/3.0;//tij + utb*(uik*(tik-tij))*(1.0/sin(wijk));
				//(tij+tik+tjk)/3.0;//tij + 

				float hijk_a = (ri+rp)*(ri+rp) - length(bijk-ai)*length(bijk-ai);

				if (hijk_a > 0.01)
				{
					//fragColor = vec4(1.0,1.0,0.0,1.0);
					//return;

					float hijk = sqrt(hijk_a);

					vec3 pijk0 = bijk - hijk*uijk;						
					vec3 pijk1 = bijk + hijk*uijk;						

					vec3 sn = cross(uij,uik);
					vec3 sp = pijk0;//-1.85*rp*cross(uij,uik);//(ai+aj+ak)/3.0-1.5*rp*cross(uij,uik);//+sn*hijk;//bijk;
					Sphere s = calcSphereIntersection(rp,nearPosition.xyz,sp,V);
							//fragColor = vec4(float(intersections[ih].id)/2.0,float(intersections[ii].id)/2.0,float(intersections[ij].id)/2.0,1.0);
							//return;

					if (s.hit)
					{
						vec3 n1 = normalize(cross(ak-ai,pijk0-ai));
						vec3 n2 = normalize(cross(ak-aj,pijk0-aj));
						vec3 n3 = normalize(cross(aj-ai,pijk0-ai));
						
						float dd = length(s.far.xyz-near.xyz);//calcDepth(s.far.xyz);
						bool bClipped = false;
						
						if (dot(s.far.xyz-ai,n1) > 0.0)
							bClipped = true;
						else if (dot(s.far.xyz-aj,n2) < 0.0)
							bClipped = true;
						else if (dot(s.far.xyz-ai,n3) < 0.0)
							bClipped = true;
						
						if (!bClipped && dd <= depth.r)
						{
							depth.r = dd;
							normal.xyz = -normalize(s.far.xyz-sp);// s.near.cenm-normalize(s.far.xyz-sp);
							patchColor = vec3(1.0,1.0,0.0);
							fragColor = vec4(1.0,0.0,1.0,1.0);

								

	//								break;
						}
						else
						{
							fragColor = vec4(1.0,0.0,0.0,1.0);
							return;
						}
					}
						
					sp = pijk1;//-1.85*rp*cross(uij,uik);//(ai+aj+ak)/3.0-1.5*rp*cross(uij,uik);//+sn*hijk;//bijk;
					s = calcSphereIntersection(rp,nearPosition.xyz,sp,V);
							//fragColor = vec4(float(intersections[ih].id)/2.0,float(intersections[ii].id)/2.0,float(intersections[ij].id)/2.0,1.0);
							//return;

					if (s.hit)
					{
						vec3 n1 = normalize(cross(ak-ai,pijk1-ai));
						vec3 n2 = normalize(cross(ak-aj,pijk1-aj));
						vec3 n3 = normalize(cross(aj-ai,pijk1-ai));
						
						float dd = length(s.far.xyz-near.xyz);//calcDepth(s.far.xyz);
						bool bClipped = false;
												
						if (dot(s.far.xyz-ai,n1) < 0.0)
							bClipped = true;
						else if (dot(s.far.xyz-aj,n2) > 0.0)
							bClipped = true;
						else if (dot(s.far.xyz-ai,n3) > 0.0)
							bClipped = true;
						
						if (!bClipped && dd <= depth.r)
						{
							depth.r = dd;
							normal.xyz = -normalize(sp-s.near.xyz);// s.near.cenm-normalize(s.far.xyz-sp);
							patchColor = vec3(1.0,0.0,0.0);

	//								break;
						}
					}
				}
				else
				{
					fragColor = vec4(1.0,0.0,0.0,1.0);
					return;
				}
			}
*/
		}

#endif
// OUTCOMMENT END


	}


//	if (entryCount > 0)
	{
		// vectors
		vec3 N = normalize(-normal.xyz);//intersections[indices[0]].center.xyz - intersections[indices[0]].near.xyz);
		vec3 L = lightPosition;
		vec3 R = normalize(reflect(L, N));
		float NdotL = abs(dot(N, L));
		float RdotV = abs(dot(R, -V));

		vec3 c = ambientMaterial + NdotL * diffuseMaterial + pow(RdotV,shininess) * specularMaterial;
		//vec3 c = color;// color.rgb*vec3(63.0/255.0,136/255.0,189/255.0);// + 0.125*color.rgb*pow(RdotV,shininess) * specularMaterial;//0.5*color.rgb + 0.5*color.rgb*(0.75+0.25 * NdotL) * diffuseMaterial + color.rgb*pow(RdotV,shininess) * specularMaterial;

		fragColor = vec4(min(vec3(1.0),c.xyz),1.0);//vec4(offset,1.0,0.0,1.0);
		fragColor.rgb *= patchColor;
		
		//if (entryCount > 0)
			//fragColor = vec4(max(intersections[0].near.w*0.5,1.0),0.0,0.0,1.0);
		
		if (overlaps > 0)
		{
		/*
			if (overlaps > 1)
				fragColor *= vec4(0.0,1.0,0.0,1.0);
			else if (overlaps == 1)
				fragColor *= vec4(1.0,0.0,0.0,1.0);
			else
				fragColor *= vec4(1.0,1.0,1.0,1.0);
				*/
		}
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



}