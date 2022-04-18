#pragma once

#include <rendering/TextureMgr.h>

#include <Camera.h>
#include <collisions/Collisions.h>

namespace Rendering
{
    enum PaintingType
    {
        TEXTURE = 1,
        COLOR,
    };

    struct Painting
    {
        PaintingType type;
        bool hasAlpha = false;
        union
        {
            glm::vec4 color;
            TextureID texture;
        };
    };

    Collisions::Ray ScreenToWorldRay(const Rendering::Camera& camera, glm::vec<2, int> screenPos, glm::vec<2, int> screenSize);

    constexpr glm::u8vec4 FloatColorToUint8(glm::vec4 color) {
        return glm::u8vec4{color * 255.f};
    }

    constexpr glm::vec4 Uint8ColorToFloat(glm::u8vec4 color){
         return glm::vec4{color} / 255.f;
    }

    constexpr glm::vec4 ColorU8ToFloat(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255){
         return glm::vec4{r, g, b, a} / 255.f;
    }
}