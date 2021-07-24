#pragma once

#include <math/Transform.h>

#include <string>

namespace Game
{
    enum DisplayType
    {
        TEXTURE,
        COLOR
    };

    struct ColorDisplay
    {
        glm::vec4 color;
    };

    struct TextureDisplay
    {
        int textureId;
    };

    union DisplayU
    {
        ColorDisplay color;
        TextureDisplay texture;
    };

    struct Display
    {
        DisplayType type;
        DisplayU display;
    };

    enum BlockType
    {
        BLOCK,
        SLOPE
    };

    struct Block
    {
        Math::Transform transform;
        BlockType type;
        Display display;
        std::string name;
    };
}