#version 330 core
out vec4 fragColor;

centroid in vec2 texCoords;

uniform int textureType;

uniform vec4 color;
uniform sampler2D uTexture;

const int TEXTURE_TYPE = 1;
const int COLOR_TYPE = 2;

void main()
{
    if(textureType == COLOR_TYPE)
    {
        fragColor = color; 
    }
    else
        fragColor = texture(uTexture, texCoords);
}