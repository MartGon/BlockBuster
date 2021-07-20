#pragma once

#include <math/Transform.h>

#include <string>

namespace Game
{
    enum BlockType
    {
        BLOCK,
        SLOPE
    };

    struct Block
    {
        Math::Transform transform;
        BlockType type;
        std::string name;
        bool enabled = true;
    };
}