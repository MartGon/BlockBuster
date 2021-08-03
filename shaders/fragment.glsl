#version 330 core
out vec4 fragColor;

in vec2 texCoords;

uniform bool hasBorder;
uniform int isPlayer;
uniform int textureType;

uniform vec4 color;
uniform vec4 borderColor;
uniform sampler2D uTexture;

const int TEXTURE_TYPE = 1;
const int COLOR_TYPE = 2;

void main()
{
    vec2 cTexCoords = texCoords - vec2(0.5, 0.5);

    if(textureType == COLOR_TYPE)
    {
        if(hasBorder && (abs(cTexCoords.x) > 0.475 || abs(cTexCoords.y) > 0.475))
        {
            fragColor = borderColor;
        }
        else
            fragColor = color;
    }
    else
        fragColor = texture(uTexture, texCoords);
}