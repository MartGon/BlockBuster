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

    enum RotType
    {
        ROT_0,
        ROT_90,
        ROT_180,
        ROT_270
    };

    struct BlockRot
    {
        RotType y = ROT_0;
        RotType z = ROT_0;
    };

    class Block
    {
    public:

        // TODO: Remove
        Math::Transform transform;

        BlockType type;
        BlockRot rot;
        Display display;

        // TODO: Remove
        std::string name;

        glm::vec3 GetRotation() const;
    };

    bool operator==(Block a, Block b);
}