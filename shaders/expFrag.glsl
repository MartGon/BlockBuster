#version 330 core
out vec4 fragColor;

centroid in vec2 texCoords;

uniform int textureType;

uniform int frameId;

uniform vec4 color;
uniform vec4 colorMod;
uniform sampler2D uTexture;

const int TEXTURE_TYPE = 1;
const int COLOR_TYPE = 2;

// Explosion color #EDB11AFF
// 0.929, 0.694, 0.101

const vec4 explosionColor = vec4(0.929, 0.694, 0.101,1.0);
const float brightFactor = 2.0f;
void main()
{

    if(textureType == COLOR_TYPE)
    {
        fragColor = color; 
    }
    else
        fragColor = texture(uTexture, texCoords);

    float lifetime = (36.0f - frameId) / 36.0f;
    fragColor = fragColor * explosionColor * brightFactor * lifetime;
    fragColor = min(vec4(1.0), fragColor);

    if(fragColor.w < 0.1)
        discard;
}