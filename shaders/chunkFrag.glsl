#version 330 core
out vec4 fragColor;

in vec2 texCoords;

uniform int textureId;
uniform sampler2DArray textureArray;

void main()
{
    fragColor = texture(textureArray, vec3(texCoords, textureId));
}