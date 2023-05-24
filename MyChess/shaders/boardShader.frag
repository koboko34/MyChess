#version 330 core

in vec3 TileColor;

out vec4 color;

uniform vec3 colorMod;

void main()
{
    color = vec4(TileColor, 1.0) * vec4(colorMod, 1.0);
}