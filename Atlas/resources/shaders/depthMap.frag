#version 330 core
in float lightDistance;
uniform float far_plane;

void main()
{
    // Mapping to [0, 1]
    gl_FragDepth = lightDistance;
}
