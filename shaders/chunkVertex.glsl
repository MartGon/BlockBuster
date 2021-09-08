#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 iTexCoords;

uniform mat4 transform;

out vec2 texCoords;

void main()
{
    vec4 worldPos = transform * vec4(pos, 1.0f);
    texCoords = iTexCoords; 
    gl_Position = worldPos;
}