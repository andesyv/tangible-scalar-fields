#version 450

layout(pixel_center_integer) in vec4 gl_FragCoord;

uniform sampler2D entryTexture;
uniform sampler2D exitTexture;
uniform sampler3D molumeTexture;

out vec4 fragColor;

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
		vec3 near = origin + min(da, ds) * l;
		vec3 far = origin + max(da, ds) * l;
		vec3 normal = normalize(near - center);

		return Sphere(true, near, far, normal);
	}
	else
	{
		return Sphere(false, vec3(0), vec3(0), vec3(0));
	}
}

void main()
{
	vec4 entry = texelFetch(entryTexture,ivec2(gl_FragCoord.xy),0);
	vec4 exit = texelFetch(exitTexture,ivec2(gl_FragCoord.xy),0);

	vec3 direction = exit.xyz-entry.xyz;
	float distance = length(direction);

	if (distance == 0.0)
		discard;

	direction /= distance;
	vec4 color = vec4(0.0,0.0,0.0,0.0);
/*
	vec3 origin = entry.xyz;
	ivec3 coord = ivec3(origin.xyz);
	ivec3 step;
	vec3 deltaDist;
	vec3 next;

	for (int i=0; i<3; ++i)
	{
		vec3 p = direction / direction[i];
		deltaDist[i] = length(p);

		if (direction[i] < 0.0)
		{
			step[i] = -1;
			next[i] = (origin[i] - coord[i]) * deltaDist[i];			
		}
		else
		{
			step[i] = 1;
			next[i] = (coord[i] + 1.0 - origin[i]) * deltaDist[i];
		}
	}

	vec4 inc = vec4(direction,1.0);
	vec4 pos = vec4(entry.xyz,0.0);

	
	while (pos.w < distance)
	{
		int side = 0;

		for (int i=1; i<3; ++i)
		{
			if (next[side] > next[i])
			{
				side = i;
			}
		}

		next[side] += deltaDist[side];
		coord[side] += step[side];

		vec4 value = texelFetch(molumeTexture,coord,0);

		if (value.w > 0.0)
		{
			vec3 spherePosition = vec3(coord)+value.xyz;

			Sphere sphere = calcSphereIntersection(0.5, origin, spherePosition, direction);
	
			if (sphere.hit)
			{		
				float shade = abs(dot(direction,sphere.normal.xyz));
				color = vec4(shade,shade,shade,1.0);
				break;
			}


		}



		pos += inc;
	}
*/

	

	vec4 inc = vec4(direction,1.0);
	vec4 pos = vec4(entry.xyz,0.0);

	do
	{
		vec4 value = texelFetch(molumeTexture,ivec3(pos),0);

		if (value.w > 0.0)
		{
			vec3 spherePosition = vec3(ivec3(pos.xyz))+value.xyz;

			Sphere sphere = calcSphereIntersection(0.5, pos.xyz, spherePosition, direction);
	
			if (sphere.hit)
			{		
				float shade = abs(dot(direction,sphere.normal.xyz));
				color = vec4(shade,shade,shade,1.0);
				break;
			}
		}

		pos += inc;
	}
	while (pos.w < distance);

	
	fragColor = color;
}