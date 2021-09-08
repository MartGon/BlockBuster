#include <TextureArray.h>

#include <stb/stb_image.h>

GL::TextureArray::TextureArray(GLsizei length, GLsizei textureSize) : length_{length}, texSize_{textureSize}
{
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &handle_);
    glBindTexture(GL_TEXTURE_2D_ARRAY, handle_);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    //glTexStorage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB, textureSize, textureSize, length);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, textureSize, textureSize, length_, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    // TODO: Set this to RGBA and convert RGB images to RGBA
}

GL::TextureArray::~TextureArray()
{
    glDeleteTextures(1, &handle_);
}

GLuint GL::TextureArray::AddTexture(std::filesystem::path folder, std::filesystem::path filename, bool flipVertically)
{
    return AddTexture(folder / filename, flipVertically);
}

GLuint GL::TextureArray::AddTexture(std::filesystem::path filepath, bool flipVertically)
{
    Bind();

    int channels;
    int sizeX, sizeY;

    stbi_set_flip_vertically_on_load(flipVertically);
    auto data = stbi_load(filepath.string().c_str(), &sizeX, &sizeY, &channels, 3);

    if(data)
    {
        auto format_ = channels == 3 ? GL_RGB : GL_RGBA;
        //glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, count_, texSize_, texSize_, length_, GL_RGB, GL_UNSIGNED_BYTE, data);
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, count_, texSize_, texSize_, 1, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

        stbi_image_free(data);
    }
    else
    {
        // TODO: User error image here
    }

    stbi_set_flip_vertically_on_load(false);

    return count_++;
}

void GL::TextureArray::Bind(GLuint activeTexture) const
{
    glActiveTexture(activeTexture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, handle_);
}