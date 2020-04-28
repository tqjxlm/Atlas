#version 330 core

uniform mat4 osg_ModelViewProjectionMatrix;

in vec4 osg_Vertex;
in vec3 gl_Normal;
out vec3 normal;

void main()
{
	normal = gl_Normal;
	gl_Position = osg_ModelViewProjectionMatrix * osg_Vertex;
}