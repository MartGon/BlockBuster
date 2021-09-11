#include <TextureArray.h>

#include <stb/stb_image.h>

GL::TextureArray::TextureArray(GLsizei length, GLsizei textureSize, int channels) : length_{length}, texSize_{textureSize}, channels_{channels}
{
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &handle_);
    glBindTexture(GL_TEXTURE_2D_ARRAY, handle_);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    auto iFormat = channels == 3 ? GL_RGB8 : GL_RGBA8;
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, iFormat, textureSize, textureSize, length);
}

GL::TextureArray::~TextureArray()
{
    glDeleteTextures(1, &handle_);
}

GL::TextureArray::TextureArray(TextureArray&& other)
{
    *this = std::move(other);
}

GL::TextureArray& GL::TextureArray::operator=(TextureArray&& other)
{
    glDeleteTextures(1, &handle_);
    this->handle_ = other.handle_;
    this->length_ = other.length_;
    this->count_ = other.count_;
    this->texSize_ = other.texSize_;
    other.handle_ = 0;

    return *this;
}

General::Result<GLuint> GL::TextureArray::AddTexture(std::filesystem::path folder, std::filesystem::path filename, bool flipVertically)
{
    return AddTexture(folder / filename, flipVertically);
}

General::Result<GLuint> GL::TextureArray::AddTexture(std::filesystem::path filepath, bool flipVertically)
{
    if(count_ >= length_)
        return General::CreateError<GLuint>("Texture Array reached maximum size");

    Bind();

    int channels;
    int sizeX, sizeY;

    stbi_set_flip_vertically_on_load(flipVertically);
    auto data = stbi_load(filepath.string().c_str(), &sizeX, &sizeY, &channels, 0);

    if(data)
    {
        // Note: 1 instead of length (before GL_RGB) because it's the amount of images to be set on this call. It's always one.
        auto format = channels_ == 3 ? GL_RGB : GL_RGBA;
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, count_, texSize_, texSize_, 1, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

        stbi_image_free(data);
    }
    else
    {
        return General::CreateError<GLuint>("Failed to load error");
    }

    stbi_set_flip_vertically_on_load(false);

    return General::CreateSuccess<GLuint>(count_++);
}

General::Result<GLuint> GL::TextureArray::AddTexture(const void* data)
{
    Bind();
    
    auto format = channels_ == 3 ? GL_RGB : GL_RGBA;
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, count_, texSize_, texSize_, 1, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

    return General::CreateSuccess<GLuint>(count_++);
}

void GL::TextureArray::Bind(GLuint activeTexture) const
{
    glActiveTexture(activeTexture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, handle_);
}

GLuint GL::TextureArray::GetHandle() const
{
    return handle_;
}