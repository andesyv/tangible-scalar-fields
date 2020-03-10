#version 450
#extension GL_ARB_shading_language_include : require
#include "/defines.glsl"

layout(location = 0) in float xValue;
layout(location = 1) in float yValue;
layout(location = 2) in float pointDiscrepancy;

out VS_OUT{
	float pointDiscrepancy;
}vs_out;

void main()
{

	vs_out.pointDiscrepancy = pointDiscrepancy;
	gl_Position = vec4(xValue, yValue, 0.0f, 1.0f);

}