#version 330 core

in vec3 tColor;
in vec2 texCoords;

uniform sampler2D uTexture;

out vec4 fragColor;

const vec4 red = vec4(1, 0, 0, 1);

void main()
{
    vec4 texColor = texture(uTexture, texCoords);
    fragColor = texColor;
}