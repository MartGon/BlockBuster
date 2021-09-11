#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in int index;
layout (location = 2) in ivec2 display;

uniform mat4 transform;

out vec2 texCoords;
flat out ivec2 iDisplay;

vec2 iTexCoords[4] = vec2[4](
    vec2(0.0f, 0.0f),
    vec2(0.0f, 1.0f),
    vec2(1.0f, 0.0f),
    vec2(1.0f, 1.0f)
);

void main()
{
    vec4 worldPos = transform * vec4(vec3(pos), 1.0f);
    texCoords = iTexCoords[index];
    iDisplay = display;
    gl_Position = worldPos;
}