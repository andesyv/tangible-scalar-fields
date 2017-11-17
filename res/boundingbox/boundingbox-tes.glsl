#version 400

layout(quads) in;
in vec3 tcPosition[];
out vec3 tePosition;
out vec4 tePatchDistance;
uniform mat4 Projection;
uniform mat4 Modelview;
uniform mat4 B, BT;

void main()
{
    float u = gl_TessCoord.x, v = gl_TessCoord.y;
    vec3 a = mix(tcPosition[0], tcPosition[1], u);
    vec3 b = mix(tcPosition[3], tcPosition[2], u);
    tePosition = mix(a, b, v);
    tePatchDistance = vec4(u, v, 1-u, 1-v);
    gl_Position = Projection * Modelview * vec4(tePosition, 1);
}