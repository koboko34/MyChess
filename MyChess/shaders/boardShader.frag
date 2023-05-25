#version 330 core

in vec3 TileColor;

out vec4 color;

uniform int colorMod;

void main()
{
    if (colorMod == 0)
    {
        color = vec4(1.0, 0.3, 0.2, 1.0);
    }
    else
    {
        color = vec4(TileColor, 1.0);
    }
}