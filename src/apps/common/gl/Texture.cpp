#include <Texture.h>

#include <stb/stb_image.h>

#include <iostream>

GL::Texture GL::Texture::FromFolder(const std::filesystem::path& folderPath, const std::string& textureName)
{
    auto texturePath = folderPath / textureName;
    return GL::Texture{texturePath};
}

GL::Texture::Texture(std::filesystem::path imagePath) : path_{imagePath}
{

}

void GL::Texture::Load(bool flipVertically)
{
    if(handle_ == 0)
    {
        int channels;
        stbi_set_flip_vertically_on_load(flipVertically);
        auto data = stbi_load(path_.c_str(), &dimensions_.x, &dimensions_.y, &channels, 0);
        if(data)
        {
            glGenTextures(1, &handle_);
            glBindTexture(GL_TEXTURE_2D, handle_);

            format_ = channels == 3 ? GL_RGB : GL_RGBA;
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dimensions_.x, dimensions_.y, 0, format_, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glBindTexture(GL_TEXTURE_2D, 0);

            stbi_image_free(data);
            stbi_set_flip_vertically_on_load(false);
        }
        else
            throw LoadError{path_, stbi_failure_reason()};
    }
    else
        throw LoadError{path_, "Texture already loaded"};
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
    this->path_ = other.path_;
    other.handle_ = 0;

    return *this;
}

void GL::Texture::Bind(unsigned int activeTexture) const
{
    glActiveTexture(activeTexture);
    glBindTexture(GL_TEXTURE_2D, handle_);
}