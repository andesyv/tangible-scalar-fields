#version 400

out vec4 FragColor;
in vec3 gFacetNormal;
in vec3 gTriDistance;
in vec4 gPatchDistance;

uniform vec3 LightPosition;
uniform vec3 DiffuseMaterial;
uniform vec3 AmbientMaterial;
uniform vec3 SpecularMaterial;
uniform float Shininess;

uniform bool edgeFlag;

const vec3 InnerLineColor = vec3(1, 1, 1);
const bool DrawLines = true;

float amplify(float d, float scale, float offset)
{
    d = scale * d + offset;
    d = clamp(d, 0, 1);
    d = 1 - exp2(-2*d*d);
    return d;
}

void main()
{
	/*
    vec3 N = normalize(gFacetNormal);
    vec3 L = LightPosition;
    vec3 E = vec3(0, 0, 1);
    vec3 H = normalize(L + E);

    float df = abs(dot(N, L));
    float sf = abs(dot(N, H));
    sf = pow(sf, Shininess);
    vec3 color = AmbientMaterial + df * DiffuseMaterial + sf * SpecularMaterial;

    float d1 = min(min(gTriDistance.x, gTriDistance.y), gTriDistance.z);
	*/
    float d2 = min(min(min(gPatchDistance.x, gPatchDistance.y), gPatchDistance.z), gPatchDistance.w);

	vec4 d = fwidth(gPatchDistance);
    vec4 s = smoothstep(vec4(0.0), d*2.5, gPatchDistance);
	float edgeFactor = min(min(s.x, s.y), min(s.z,s.w));

	if (edgeFactor >= 1.0)
		discard;

	if(gl_FrontFacing)
	{
		FragColor = vec4(1.0, 1.0, 1.0, (1.0-edgeFactor)*0.95);
	}
	else
	{
		FragColor = vec4(1.0, 1.0, 1.0, (1.0-edgeFactor)*0.7);
	}
}