#version 330 core
out vec4 fragColor;

in vec2 texCoords;

uniform bool hasBorder;
uniform vec4 borderColor;

uniform int textureType;
uniform int textureId;

uniform sampler2DArray textureArray;
uniform sampler2DArray colorArray;

const int TEXTURE_TYPE = 0;
const int COLOR_TYPE = 1;

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
            fragColor = texture(colorArray, vec3(texCoords, textureId));
    }
    else
        fragColor = texture(textureArray, vec3(texCoords, textureId));
}