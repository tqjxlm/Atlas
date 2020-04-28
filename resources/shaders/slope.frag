#version 330 core

in vec3 normal;
out vec4 fragColor;
uniform vec4 slopeColors[5];

vec4 Normal2Gray(vec3 n) {
	float degree = abs(acos((n.z / length(n)))) * 180 / 3.14159;
	vec4 color;
	if (degree < 2)
		color = slopeColors[0];
	else if (degree < 6)
		color = slopeColors[1];
	else if (degree < 15)
		color = slopeColors[2];
	else if (degree < 30)
		color = slopeColors[3];
	else
		color = slopeColors[4];
	return color;
}

void main()
{
	fragColor = Normal2Gray(normal);
}