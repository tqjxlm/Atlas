#version 330 core
out vec2 texCoords;
out vec3 fragPos;
void main()
{
    texCoords = gl_MultiTexCoord0.st;
    texCoords2 = gl_MultiTexCoord1.st;
    gl_Position = ftransform();
    fragPos = gl_Position.xyz;
}