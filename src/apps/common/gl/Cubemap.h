#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <unordered_map>
#include <filesystem>

namespace GL
{
    class Cubemap
    {
    public:
        enum Face
        {
            RIGHT,
            LEFT,
            TOP,
            BOTTOM,
            FRONT,
            BACK,

            COUNT
        };

        Cubemap();
        ~Cubemap();

        Cubemap(const Cubemap&) = delete;
        Cubemap& operator=(const Cubemap&) = delete;

        Cubemap(Cubemap&&);
        Cubemap& operator=(Cubemap&&);

        using TextureMap = std::unordered_map<Face, std::filesystem::path>;
        void Load(TextureMap texturePaths, bool flipVertically = false);
        void Bind(unsigned int activeTexture = GL_TEXTURE0) const;

    private:
        uint32_t handle_ = -1;

        bool loaded = false;
        glm::ivec2 dimensions_{0, 0};
        int format_ = 0;
    };
}