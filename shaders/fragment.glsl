#version 330 core
out vec4 fragColor;

in vec3 fragPos;
in vec2 texCoords;

uniform int isPlayer;
uniform sampler2D uTexture;

void main()
{
    if(isPlayer == 1)
        fragColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    else
        fragColor = texture(uTexture, texCoords);
}