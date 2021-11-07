#version 330 core
out vec4 fragColor;

uniform vec4 color;
uniform bool dmg;

in vec3 tColor;

void main()
{
    if(dmg)
        fragColor = color;
    else
        fragColor = vec4(tColor, 1);
}