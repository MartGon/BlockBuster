#version 330 core

in vec3 tColor;
out vec4 fragColor;

uniform vec4 color;
uniform bool dmg;

const vec4 red = vec4(1, 0, 0, 1);

void main()
{
    if(dmg)
        fragColor = red;
    else
        fragColor = color;
}