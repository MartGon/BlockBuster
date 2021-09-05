#pragma once

#include <math/Transform.h>

#include <string>

namespace Game
{
    enum DisplayType : uint8_t
    {
        TEXTURE,
        COLOR
    };

    struct Display
    {
        DisplayType type;
        int id;
    };

    enum BlockType : uint8_t
    {
        NONE,
        BLOCK,
        SLOPE
    };

    enum RotType : int8_t
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

        BlockType type;
        BlockRot rot;
        Display display;

        glm::vec3 GetRotation() const;
    };

    // TODO: 270 on z to 90 with Y rot
    glm::vec3 BlockRotToVec3(BlockRot rot);
    Math::Transform GetBlockTransform(const Block& block, glm::ivec3 pos, float blockScale);
    Math::Transform GetBlockTransform(glm::ivec3 pos, float blockScale);
    Math::Transform GetBlockTransform(glm::ivec3 pos, Game::BlockRot rot, float blockScale);
}