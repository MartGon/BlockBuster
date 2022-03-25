#include <Cubemap.h>

#include <Texture.h>

#include <stb/stb_image.h>

using namespace GL;

Cubemap::Cubemap()
{
    glGenTextures(1, &handle_);
}

Cubemap::~Cubemap()
{
    glDeleteTextures(1, &handle_);
}

Cubemap::Cubemap(Cubemap&& other)
{
    *this = std::move(other);
}

Cubemap& Cubemap::operator=(Cubemap&& other)
{
    std::swap(handle_, other.handle_);
    this->loaded = other.loaded;
    this->dimensions_ = other.dimensions_;
    this->format_ = other.format_;

    return *this;
}

void Cubemap::Load(Cubemap::TextureMap texturePaths, bool flipVertically)
{
    if(loaded)
        throw LoadError{"Cubemap already loaded"};

    if(texturePaths.size() != Face::COUNT)
        throw LoadError{"Cubemap texture path has incorrect number of faces"};

    stbi_set_flip_vertically_on_load(flipVertically);

    Bind();
    for(int i = Face::RIGHT; i < Face::COUNT; i++)
    {
        auto path = texturePaths[(Face)i];

        int channels;
        auto data = stbi_load(path.string().c_str(), &dimensions_.x, &dimensions_.y, &channels, 0);
        if(data)
        {
            format_ = channels == 3 ? GL_RGB : GL_RGBA;
            auto face = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;
            glTexImage2D(face, 0, GL_RGBA, dimensions_.x, dimensions_.y, 0, format_, GL_UNSIGNED_BYTE, data);

            stbi_image_free(data);
        }
        else
        {
            stbi_image_free(data);

            throw LoadError{stbi_failure_reason()};
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    loaded = true;
}

void Cubemap::Bind(unsigned int activeTexture) const
{
    glActiveTexture(activeTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, handle_);
};