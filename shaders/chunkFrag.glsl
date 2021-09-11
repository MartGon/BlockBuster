#version 330 core
out vec4 fragColor;

in vec2 texCoords;

uniform int textureType;
uniform int textureId;
uniform sampler2DArray textureArray;
uniform sampler2DArray colorArray;

const int TEXTURE_TYPE_IMAGE = 0;
const int TEXTURE_TYPE_COLOR = 1;

void main()
{
    if(textureType == TEXTURE_TYPE_IMAGE)
        fragColor = texture(textureArray, vec3(texCoords, textureId));
    else if(textureType == TEXTURE_TYPE_COLOR)
        fragColor = texture(colorArray, vec3(texCoords, textureId));
}