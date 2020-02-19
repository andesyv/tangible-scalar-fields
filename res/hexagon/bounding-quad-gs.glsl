#version 450

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

//[x,y]
uniform vec2 maxBounds;
uniform vec2 minBounds;

uniform int windowHeight;
uniform int windowWidth;

uniform mat4 modelViewProjectionMatrix;

out vec4 boundsScreenSpace;

void main()
{
    //calc bounding box coordinates in NDC Space
    vec4 maxBoundClip = modelViewProjectionMatrix * vec4(maxBounds,0.0,1.0);
    vec2 maxBoundNDC = maxBoundClip.xy/maxBoundClip.w;

    vec4 minBoundClip = modelViewProjectionMatrix * vec4(minBounds,0.0,1.0);
    vec2 minBoundNDC = minBoundClip.xy/minBoundClip.w;

	// get bounding box coordinates in Screen Space
    boundsScreenSpace = vec4(windowWidth/2*maxBoundNDC[0]+windowWidth/2, //maxX
	windowHeight/2*maxBoundNDC[1]+windowHeight/2, //maxY
	windowWidth/2*minBoundNDC[0]+windowWidth/2, //minX
	windowHeight/2*minBoundNDC[1]+windowHeight/2); //minY

    // create bounding box geometry
	vec4 pos = modelViewProjectionMatrix * (gl_in[0].gl_Position + vec4(maxBounds[0],maxBounds[1],0.0,0.0));
	gl_Position = pos;
	EmitVertex();

	pos = modelViewProjectionMatrix * (gl_in[0].gl_Position + vec4(minBounds[0],maxBounds[1],0.0,0.0));
	gl_Position = pos;
	EmitVertex();

	pos = modelViewProjectionMatrix * (gl_in[0].gl_Position + vec4(maxBounds[0],minBounds[1],0.0,0.0));
	gl_Position = pos;
	EmitVertex();

	pos = modelViewProjectionMatrix * (gl_in[0].gl_Position + vec4(minBounds[0],minBounds[1],0.0,0.0));
	gl_Position = pos;
	EmitVertex();

	EndPrimitive();
}