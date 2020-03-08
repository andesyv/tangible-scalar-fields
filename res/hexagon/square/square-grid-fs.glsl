#version 450
#include "/defines.glsl"
#include "/globals.glsl"

layout (location = 0) out vec4 squareGridTexture;

in vec2 origMaxBoundScreenSpace;

uniform vec3 squareBorderColor;

uniform int windowHeight;
uniform int windowWidth;

void main()
{

	// we don't render pixels outside the original bounding box
    if(gl_FragCoord.x > origMaxBoundScreenSpace[0] || gl_FragCoord.y > origMaxBoundScreenSpace[1]){
        discard;
    }

	squareGridTexture = vec4(squareBorderColor, 1.0f);
}