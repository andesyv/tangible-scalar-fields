#version 400

in vec4 position;
uniform float time;
out int vertexId;

highp float rand(vec2 co)
{
    highp float a = 12.9898;
    highp float b = 78.233;
    highp float c = 43758.5453;
    highp float dt= dot(co.xy ,vec2(a,b));
    highp float sn= mod(dt,3.14);
    return fract(sin(sn) * c);
}

void main()
{
	//float rand = rand(position.xy);
	gl_Position = vec4(position);//vec4(position.xyz+vec3(sin(3.0*(time+rand)),cos(time*rand*0.5),sin(3.0*(time-rand))),1.0);

	vertexId = gl_VertexID;
}