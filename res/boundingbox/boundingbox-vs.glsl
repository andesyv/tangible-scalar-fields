#version 400

in vec3 Position;
out vec3 vPosition;

void main()
{
    vPosition = Position.xyz;
}