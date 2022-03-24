#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

namespace GL
{
    class Cubemap
    {
    public:
        Cubemap();
        ~Cubemap();

        Cubemap(const Cubemap&) = delete;
        Cubemap& operator=(const Cubemap&) = delete;

        Cubemap(Cubemap&&);
        Cubemap& operator=(Cubemap&&);

    private:
        uint32_t handle_ = -1;
    };
}