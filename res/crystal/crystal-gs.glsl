#version 450

layout(points) in;
layout(triangle_strip, max_vertices = 12) out;

in VS_OUT {
    uint id;
    ivec2 texPos;
    float value;
    vec3 vertices[7];
} gs_in[];

uniform float tile_scale = 0.6;
uniform mat4 MVP = mat4(1.0);

const float PI = 3.1415926;

out GS_OUT {
    flat uint id;
    flat float value;
} gs_out;

// i = 0 => right Center
// continue agains clock circle (i = 1 = right Top)
void emitHexagonVertex(int i){
    //flat-topped
    float angle_deg = -60.0 * i;
    float angle_rad = PI / 180.0 * angle_deg;

    // Offset from center of point + grid width
    vec4 offset = vec4(tile_scale * cos(angle_rad), tile_scale * sin(angle_rad), 0.0, 0.0);
    gl_Position = MVP * (gl_in[0].gl_Position + offset);

    EmitVertex();
}

void emitHexagonCenterVertex(){
    gl_Position = MVP * gl_in[0].gl_Position;
    EmitVertex();
}

void main()
{
    gs_out.id = gs_in[0].id;
    gs_out.value = gs_in[0].value;

    gl_Position = MVP * vec4(gs_in[0].vertices[1], 1.0);
    EmitVertex();
    gl_Position = MVP * vec4(gs_in[0].vertices[0], 1.0);
    EmitVertex();
    gl_Position = MVP * vec4(gs_in[0].vertices[2], 1.0);
    EmitVertex();
    gl_Position = MVP * vec4(gs_in[0].vertices[3], 1.0);
    EmitVertex();
    EndPrimitive();

    gl_Position = MVP * vec4(gs_in[0].vertices[3], 1.0);
    EmitVertex();
    gl_Position = MVP * vec4(gs_in[0].vertices[0], 1.0);
    EmitVertex();
    gl_Position = MVP * vec4(gs_in[0].vertices[4], 1.0);
    EmitVertex();
    gl_Position = MVP * vec4(gs_in[0].vertices[5], 1.0);
    EmitVertex();
    EndPrimitive();

    gl_Position = MVP * vec4(gs_in[0].vertices[5], 1.0);
    EmitVertex();
    gl_Position = MVP * vec4(gs_in[0].vertices[0], 1.0);
    EmitVertex();
    gl_Position = MVP * vec4(gs_in[0].vertices[6], 1.0);
    EmitVertex();
    gl_Position = MVP * vec4(gs_in[0].vertices[1], 1.0);
    EmitVertex();
    EndPrimitive();
}