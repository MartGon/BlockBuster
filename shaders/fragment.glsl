#version 330 core
out vec4 fragColor;
in vec3 fragPos;

void main()
{
    fragColor = vec4(fragPos, 1.0f);
}