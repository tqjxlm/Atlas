#version 330 core
#extension GL_NV_shadow_samplers_cube : enable
out vec4 FragColor;

in vec3 texCoords;

uniform samplerCube depthMap;
uniform samplerCube colorMap;

const float near_plane = 0.1;
const float far_plane = 1000.0;

float LinearizeDepth(float z) 
{ 
	float z_n = 2.0 * z - 1.0; 
    return 2.0 * near_plane * far_plane / (far_plane + near_plane - z_n * (far_plane - near_plane)); 
}; 

void main()
{
    // vec3 color = vec3(LinearizeDepth(textureCube(depthMap, texCoords).r)) / far_plane;
    vec3 color = vec3(textureCube(depthMap, texCoords).r);
    // vec3 color = vec3(textureCube(colorMap, texCoords));

    FragColor = vec4(color, 1.0);
}
