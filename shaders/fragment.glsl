#version 330 core
out vec4 fragColor;

in vec2 texCoords;

uniform int isPlayer;
uniform int textureType;
uniform vec4 color;
uniform sampler2D uTexture;

const int TEXTURE_TYPE = 1;
const int COLOR_TYPE = 2;

void main()
{
    vec2 cTexCoords = texCoords - vec2(0.5, 0.5);

    if(textureType == COLOR_TYPE)
    {
        if(abs(cTexCoords.x) > 0.475 || abs(cTexCoords.y) > 0.475)
        {
            vec4 negColor = abs(color  - vec4(1.0f));
            float darkness = (color.r + color.g + color.b) / 3.0;
            if(darkness < 0.5)
                fragColor = vec4(1.0);
            else
                fragColor = vec4(0.0);
        }
        else
            fragColor = color;
    }
    else
        fragColor = texture(uTexture, texCoords);
}