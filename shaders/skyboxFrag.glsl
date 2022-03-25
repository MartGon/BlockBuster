#version 330 core

in vec3 texCoords;

uniform samplerCube skybox;

out vec4 FragColor;

void main()
{
    FragColor = texture(skybox, texCoords);
    //FragColor = vec4(texCoords, 1.0);
}