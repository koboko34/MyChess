#version 330 core

in vec3 TileColor;

out vec4 color;

void main()
{
    color = vec4(TileColor, 1.0);
}