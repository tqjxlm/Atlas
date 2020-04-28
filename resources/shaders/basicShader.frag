#version 330 core

out vec4 FragColor;

in vec2 texCoords;

uniform sampler2D baseTexture;

void main()
{
    vec3 color = texture(baseTexture, texCoords).rgb;

    FragColor = vec4(color, 1.0);
}