#pragma once
#include <glad/glad.h>

#include <glm/glm.hpp>

#include <filesystem>
#include <exception>

namespace GL
{

    class Texture
    {
    public:
        Texture();
        ~Texture();

        Texture(const Texture& other) = delete;
        Texture& operator=(const Texture& other) = delete;

        Texture(Texture&& other);
        Texture& operator=(Texture&& other);

        void LoadFromFolder(std::filesystem::path folder, std::filesystem::path textureName, bool flipVertically = false);
        void LoadFromMemory(void* data, size_t bufferSize, bool flipVertically = false);
        void Load(std::filesystem::path texturePath, bool flipVertically = false);
        void Load(uint8_t* data, glm::ivec2 size, int format);
        void Bind(unsigned int activeTexture = GL_TEXTURE0) const;

        bool IsLoaded() const
        {
            return loaded;
        }

        unsigned int GetGLId() const
        {
            return handle_;
        }

        inline glm::ivec2 GetSize()
        {
            return dimensions_;
        }
        
    private:

        unsigned int handle_ = 0;
        glm::ivec2 dimensions_{0, 0};
        int format_ = 0;

        bool loaded = false;
    };

    // Exceptions
    class LoadError : public std::runtime_error
    {
    public:
        LoadError(const char* msg) : runtime_error(msg) {}
    };

    class LoadTextureError : public std::runtime_error
    {
    public:
        LoadTextureError(std::filesystem::path path, const char* msg) : path_{path}, runtime_error(msg) {}
    
        const std::filesystem::path path_;
    };
}