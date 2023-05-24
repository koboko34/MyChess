#version 330 core

uniform uint objectId;
uniform uint drawId;

out uvec3 FragColor;

void main()
{
    FragColor = uvec3(objectId, drawId, gl_PrimitiveID);
}