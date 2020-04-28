#version 330 core

in vec4 gl_Vertex;

uniform mat4 osg_ModelViewMatrix;
uniform mat4 osg_ViewMatrixInverse;
uniform vec3 center;

out vec3 texCoords;

void main()
{
    vec3 worldPos = (osg_ViewMatrixInverse * osg_ModelViewMatrix * gl_Vertex).xyz;
    texCoords = normalize(worldPos - center);
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
