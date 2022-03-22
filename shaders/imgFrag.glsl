#version 330 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D uTexture;
uniform vec4 uColor;

void main()
{    
    color = texture(uTexture, TexCoords);
    color = vec4(1.0);
}  