#version 330 core
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normals;
layout (location = 2) in vec3 texCoords;
out vec2 TexCoords;

uniform vec2 offset;
uniform vec2 scale;

void main()
{
    vec2 normalizedPos = (vertex.xy + offset.xy)* scale;
    gl_Position = vec4(normalizedPos.xy, 0.0, 1.0);
    TexCoords = texCoords.xy;
}  