#version 330 core
layout (location = 0) in vec3 pos;

uniform mat4 transform;

out vec3 texCoords;

void main()
{
    texCoords = pos;
    vec4 worldPos = transform * vec4(pos, 1.0f);
    gl_Position = worldPos.xyww; // This is done to avoid disabling Depth testing
}