#version 330 core

in vec4 gl_Vertex;
in vec3 gl_Normal;
in vec2 gl_MultiTexCoord0;

uniform mat4 osg_ModelViewProjectionMatrix; 
uniform mat4 osg_ViewMatrixInverse;
uniform vec3 lightPos;

out vec3 worldPos;
out vec3 normal;
out vec2 texCoords;
out float lightDistance;

void main()
{
    worldPos = (osg_ViewMatrixInverse * gl_ModelViewMatrix * gl_Vertex).xyz;
    lightDistance = length(worldPos - lightPos);

	normal = normalize( gl_Normal );
    texCoords = gl_MultiTexCoord0;
    gl_Position = osg_ModelViewProjectionMatrix * gl_Vertex;
}
