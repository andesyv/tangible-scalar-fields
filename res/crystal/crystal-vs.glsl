#version 450

in vec3 inPos;

uniform int num_cols = 4;
uniform mat4 MVP = mat4(1.0);
uniform float tile_spacing = 1.0;

flat out uint vs_id;

void main() {
    //calculate position of hexagon center - in double height coordinates!
    //https://www.redblobgames.com/grids/hexagons/#coordinates-doubled
    float row;
    float col = mod(gl_VertexID,num_cols);

    if(mod(col,2) == 0){
        row = floor(gl_VertexID/num_cols) * 2;
    }
    else{
        row = (floor(gl_VertexID/num_cols) * 2) - 1;
    }

    vs_id = gl_VertexID;

//    mat4 trans = mat4(1.0);
//    trans[3].xyz = vec3(col, row, 0.0);

//    //position of hexagon in accumulate texture (offset coordinates)
//    vs_out.accTexPosition = vec2(col, floor(gl_VertexID/num_cols));
//
//    gl_Position =  vec4(col * horizontal_space + minBounds_Off.x + tileSize, row * vertical_space/2.0f + minBounds_Off.y + vertical_space, 0.0f, 1.0f);

    gl_Position = vec4(col * (tile_spacing * (7.0 / 4.0)), row * tile_spacing, 0.0, 1.0);
}