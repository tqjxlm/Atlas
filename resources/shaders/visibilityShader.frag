#version 330 core
out vec4 FragColor;

in vec3 worldPos;
in vec3 normal;
in vec2 texCoords;
in float lightDistance;

uniform vec3 lightPos;
uniform vec4 visibleColor;
uniform vec4 invisibleColor;

uniform sampler2D baseTexture;
uniform samplerCube shadowMap;

uniform float near_plane;
uniform float far_plane;
uniform float user_area;

float linearizeDepth(float z) 
{ 
	float z_n = 2.0 * z - 1.0; 
    return 2.0 * near_plane * far_plane / (far_plane + near_plane - z_n * (far_plane - near_plane)); 
}; 

// Return 1 for shadowed, 0 visible
bool isShadowed(vec3 lightDir)
{
    float bias = max(0.01 * (1.0 - dot(normal, lightDir)), 0.001) * far_plane;

	float z = linearizeDepth(texture(shadowMap, lightDir).r);
	return lightDistance - bias > z;
}

void main()
{
    vec3 baseColor = texture(baseTexture, texCoords).rgb;
	
	if (length(lightPos.xy - worldPos.xy) > user_area)
		FragColor = vec4(baseColor, 1.0);
	else
	{
		vec3 lightDir = normalize(lightPos - worldPos);
		float normDif = max(dot(lightDir, normal), 0.0);

		vec3 lighting;
		// Render as visible only if it can be seen by light source
		if (normDif > 0.0 && isShadowed(-lightDir) == false)
			lighting = visibleColor.rgb * baseColor;
		else
			lighting = invisibleColor.rgb * baseColor;

		FragColor = vec4(lighting, 1.0);
	}
}
