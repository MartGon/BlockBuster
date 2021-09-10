
// Texture Array Implementation
// Notes:
// - ImGui has to be modified in order to use TextureArrays. Check Bookmarks, but basically a struct has to be passed to Image(). That
//   struct should hold the texture id (GLuint) and the layer. Maybe a GLuint id is generated each time  glTexSubImage3D is called. Needs testing. It may be global
//   or equal to layer level.
// - TextureArrays need to load each texture individually from data. So a (Load/Add)Texture has to be created in it
// 

#pragma once
#include <glad/glad.h>

#include <filesystem>

namespace GL
{
    class TextureArray
    {
    public:
        TextureArray(GLsizei length, GLsizei textureSize);
        ~TextureArray();

        TextureArray(const TextureArray&) = delete;
        TextureArray& operator=(const TextureArray&) = delete;

        TextureArray(TextureArray&& other);
        TextureArray& operator=(TextureArray&& other);

        GLuint AddTexture(std::filesystem::path folder, std::filesystem::path filename, bool flipVertically = true);
        GLuint AddTexture(std::filesystem::path filepath, bool flipVertically = true);

        void Bind(GLuint activeTexture = GL_TEXTURE0) const;

        GLuint GetHandle() const;

    private:
        GLsizei length_;
        GLsizei count_ = 0;

        GLsizei texSize_;

        GLuint handle_;
    };
}