#version 330 core
out vec4 fragColor;
in vec3 fragPos;

uniform int isPlayer;

void main()
{
    fragColor = vec4(fragPos * (1 - isPlayer), 1.0f);
}