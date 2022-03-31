#include <Texture.h>

#include <stb/stb_image.h>

#include <iostream>


GL::Texture::Texture()
{
    glGenTextures(1, &handle_);
    glBindTexture(GL_TEXTURE_2D, handle_);
}

void GL::Texture::LoadFromFolder(std::filesystem::path folder, std::filesystem::path textureName, bool flipVertically)
{
    Load(folder / textureName, flipVertically);
}

void GL::Texture::LoadFromMemory(void* buffer, size_t bufferSize, bool flipVertically)
{
    stbi_set_flip_vertically_on_load(flipVertically);
    int channels;
    auto data = stbi_load_from_memory((const stbi_uc*)buffer, bufferSize, &dimensions_.x, &dimensions_.y, &channels, 4);
    if(data)
    {
        format_ = GL_RGBA;
        Load(data, dimensions_, format_);

        stbi_image_free(data);
        stbi_set_flip_vertically_on_load(false);
    }
}

void GL::Texture::Load(std::filesystem::path texturePath, bool flipVertically)
{
    if(loaded)
        throw LoadTextureError{texturePath, "Texture already loaded"};

    int channels;
    stbi_set_flip_vertically_on_load(flipVertically);
    auto data = stbi_load(texturePath.string().c_str(), &dimensions_.x, &dimensions_.y, &channels, 4);
    if(data)
    {
        format_ = GL_RGBA;
        Load(data, dimensions_, format_);

        stbi_image_free(data);
        stbi_set_flip_vertically_on_load(false);
    }
    else
        throw LoadTextureError{texturePath, stbi_failure_reason()};   
}

void GL::Texture::Load(uint8_t* data, glm::ivec2 size, int format)
{
    Bind();
    dimensions_ = size;
    format_ = format;

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0);

    loaded = true;
}

GL::Texture::~Texture()
{
    glDeleteTextures(1, &handle_);
}

GL::Texture::Texture(Texture&& other)
{
    *this = std::move(other);
}

GL::Texture& GL::Texture::operator=(Texture&& other)
{
    glDeleteTextures(1, &handle_);
    this->handle_ = other.handle_;
    this->dimensions_ = other.dimensions_;
    this->format_ = other.format_;
    this->loaded = other.loaded;
    other.handle_ = 0;

    return *this;
}

void GL::Texture::Bind(unsigned int activeTexture) const
{
    glActiveTexture(activeTexture);
    glBindTexture(GL_TEXTURE_2D, handle_);
}