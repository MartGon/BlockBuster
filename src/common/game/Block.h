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
        BLOCK,
        SLOPE
    };

    class Block
    {
    public:

        Math::Transform transform;
        BlockType type;
        Display display;
        std::string name;
    };

    bool operator==(Block a, Block b);
}