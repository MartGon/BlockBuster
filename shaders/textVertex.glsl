#version 330 core
layout (location = 0) in vec2 vertex;
layout (location = 1) in vec2 texCoords;
out vec2 TexCoords;

uniform vec2 offset;
uniform vec2 scale;

void main()
{
    vec2 normalizedPos = (vertex + offset) * scale;
    gl_Position = vec4(normalizedPos.xy, 0.0, 1.0);
    TexCoords = texCoords;
}  