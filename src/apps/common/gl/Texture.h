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

        static Texture FromFolder(const std::filesystem::path& folderPath, const std::string& textureName);

        Texture(std::filesystem::path imagePath);
        ~Texture();

        Texture(Texture& other) = delete;
        Texture& operator=(const Texture& other) = delete;

        Texture(Texture&& other);
        Texture& operator=(Texture&& other);

        void Load(bool flipVertically = false);
        void Bind(unsigned int activeTexture = GL_TEXTURE0) const;

        // Exceptions
        class LoadError : public std::runtime_error
        {
        public:
            LoadError(std::filesystem::path path, const char* msg) : path_{path}, runtime_error(msg) {}
        
            const std::filesystem::path path_;
        };

    private:

        std::filesystem::path path_;
        unsigned int handle_ = 0;
        glm::ivec2 dimensions_{0, 0};
        int format_ = 0;
    };
}