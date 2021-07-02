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
        Texture(std::filesystem::path imagePath);
        ~Texture();

        Texture(Texture& other) = delete;
        Texture& operator=(const Texture& other) = delete;

        Texture(Texture&& other);
        Texture& operator=(Texture&& other);

        void Bind();

        // Exceptions
        class LoadError : public std::runtime_error
        {
        public:
            LoadError(const char* msg) : runtime_error(msg) {}
        };

    private:

        unsigned int handle_;
        glm::ivec2 dimensions_;
        int format_;
    };
}