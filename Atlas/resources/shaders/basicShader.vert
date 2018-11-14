#version 330 core

out vec2 texCoords;

void main()
{
    texCoords = gl_MultiTexCoord0;
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
