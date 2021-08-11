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

    struct Display
    {
        DisplayType type;
        int id;
    };

    enum BlockType
    {
        NONE,
        BLOCK,
        SLOPE
    };

    class Block
    {
    public:

        // TODO: Remove
        Math::Transform transform;

        BlockType type;
        Display display;

        // TODO: Remove
        std::string name;
    };

    bool operator==(Block a, Block b);
}