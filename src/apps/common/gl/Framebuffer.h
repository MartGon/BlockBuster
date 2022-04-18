#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <Texture.h>

namespace GL
{
    class Framebuffer
    {
    public:
        Framebuffer() = default;
        ~Framebuffer();

        Framebuffer(const Framebuffer&) = delete;
        Framebuffer& operator=(const Framebuffer&) = delete;

        Framebuffer(Framebuffer&& other);
        Framebuffer& operator=(Framebuffer&& other);

        void Init(glm::ivec2 texSize);

        void Bind();
        void Unbind();

        void BindTexture(unsigned int activeTexture = GL_TEXTURE0);
        
        inline GLuint GetTexHandle()
        {
            return texture;
        }

        inline glm::ivec2 GetTextureSize()
        {
            return size;
        }

    private:
        GLuint fbo;
        GLuint rbo;
        GLuint texture;
        glm::ivec2 size{0, 0};
        bool loaded = false;
    };
}