#version 330 core

uniform uint objectId;
uniform uint drawId;

out uvec4 FragColor;

void main()
{
    FragColor = uvec4(objectId, drawId, gl_PrimitiveID, 1);
}