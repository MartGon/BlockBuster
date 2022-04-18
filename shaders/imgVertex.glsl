#version 330 core
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normals;
layout (location = 2) in vec3 texCoords;
out vec2 TexCoords;

uniform vec2 offset;
uniform vec2 size;
uniform vec2 screenSize;
uniform float rot;

void main()
{
    //vec2 scale = size / screenSize;
    vec2 pixelVertex = (vertex.xy * 2.0f) * size;
    vec2 rotPixelVertex;
    rotPixelVertex.x = cos(rot) * pixelVertex.x - sin(rot) * pixelVertex.y;
    rotPixelVertex.y = sin(rot) * pixelVertex.x + cos(rot) * pixelVertex.y; 
    vec2 pixelPos = rotPixelVertex + offset * 2.0f;
    
    vec2 normalizedPos = pixelPos / screenSize;
    gl_Position = vec4(normalizedPos.xy, 0.0, 1.0);
    TexCoords = texCoords.xy;
}  