#version 400

in vec3 position;
in vec3 pagePosition;
out vec3 vPosition;

void main()
{
    vPosition = position.xyz+pagePosition.xyz;
}