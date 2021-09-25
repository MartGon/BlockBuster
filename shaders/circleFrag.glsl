#version 330 core
out vec4 fragColor;

uniform vec4 color;

in vec3 tColor;

void main()
{
    fragColor = vec4(tColor, 1);
}