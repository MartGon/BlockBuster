#version 330 core
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 iTexCoords;

uniform vec3 center;
uniform vec3 camRight;
uniform vec3 camUp;
uniform vec2 scale;
uniform mat4 projView;

uniform vec4 colorMod;

out vec2 texCoords;

void main()
{
    vec3 vertexPos = center + camRight * vertex.x * scale.x + camUp * vertex.y * scale.y;
    gl_Position = projView * vec4(vertexPos, 1.0);

    texCoords = iTexCoords;
    
}