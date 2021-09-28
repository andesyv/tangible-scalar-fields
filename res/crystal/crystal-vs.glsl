#version 450

in vec3 inPos;

uniform int num_cols = 4;
uniform int num_rows = 4;
uniform mat4 MVP = mat4(1.0);
uniform float horizontal_space = 1.0;
uniform float vertical_space = 1.0;
uniform mat4 disp_mat = mat4(1.0);

out VS_OUT {
    flat uint id;
    flat ivec2 texPos;
} vs_out;

void main() {
    //calculate position of hexagon center - in double height coordinates!
    //https://www.redblobgames.com/grids/hexagons/#coordinates-doubled

    float col = mod(gl_InstanceID,num_cols);
    float row = floor(gl_InstanceID/num_cols) * 2.0 - (mod(col,2) == 0 ? 0.0 : 1.0);

    vs_out.id = gl_InstanceID;

    //position of hexagon in accumulate texture (offset coordinates)
    vs_out.texPos = ivec2(col, floor(gl_InstanceID/num_cols));
//
//    gl_Position =  vec4(col * horizontal_space + minBounds_Off.x + tileSize, row * vertical_space/2.0f + minBounds_Off.y + vertical_space, 0.0f, 1.0f);

//    gl_Position = vec4((col - 0.5 * (num_cols-1)) * horizontal_space, vertical_space * 0.5 * (row - num_rows + (num_cols != 1 ? 1.5 : 1.0)), 0.0, 1.0);
    gl_Position = disp_mat * vec4(col, row, 0.0, 1.0);
}