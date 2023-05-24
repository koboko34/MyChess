#version 330 core

uniform uint objectIndex;
uniform uint drawIndex;

out uvec3 FragColor;

void main()
{
    FragColor = uvec3(objectIndex, drawIndex, gl_PrimitiveID);
}