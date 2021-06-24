#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;

uniform mat4 transform;
out vec3 fragPos;

void main()
{
    vec4 worldPos = transform * vec4(pos, 1.0f);
    fragPos = pos;  
    gl_Position = worldPos;
}