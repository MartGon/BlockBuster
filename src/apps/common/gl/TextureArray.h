#pragma once
#include <glad/glad.h>

#include <filesystem>

#include <util/Result.h>

namespace GL
{
    class TextureArray
    {
    public:
        TextureArray(GLsizei length) : length_{length} {};
        TextureArray(GLsizei length, GLsizei textureSize, GLsizei channels = 3);
        ~TextureArray();

        TextureArray(const TextureArray&) = delete;
        TextureArray& operator=(const TextureArray&) = delete;

        TextureArray(TextureArray&& other);
        TextureArray& operator=(TextureArray&& other);

        Util::Result<GLuint> AddTexture(std::filesystem::path folder, std::filesystem::path filename, bool flipVertically = true);
        Util::Result<GLuint> AddTexture(std::filesystem::path filepath, bool flipVertically = true);
        Util::Result<GLuint> AddTexture(const void* data);
        void SetTexture(GLuint id, const void* data);

        void Bind(GLuint activeTexture = GL_TEXTURE0) const;

        GLuint GetHandle() const;

        inline GLsizei GetLength()
        {
            return length_;
        }

        inline GLsizei GetTextureSize()
        {
            return texSize_;
        }

        inline GLsizei GetChannels()
        {
            return channels_;
        }

    private:

        void Init();
        bool IsInitialized();

        GLsizei length_;
        GLsizei count_ = 0;

        GLsizei channels_ = 0;
        GLsizei texSize_ = 0;

        GLuint handle_ = -1;
    };
}