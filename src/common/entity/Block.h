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
    
    struct TextureOptions
    {

    };

    struct ColorOptions
    {
        bool showBorder = false;
    };

    union DisplayOptions
    {
        TextureOptions texture;
        ColorOptions color;
    };

    struct Display
    {
        DisplayType type = TEXTURE;
        unsigned int id = 0;
        //DisplayOptions options;
        
    };
    bool operator==(Display a, Display b);
    bool operator!=(Display a, Display b);

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
        ROT_270,
        ROT_MAX
    };

    enum class RotationAxis
    {
        X,
        Y,
        Z
    };

    struct BlockRot 
    {
        RotType y = ROT_0;
        RotType z = ROT_0;
    };
    bool operator==(BlockRot a, BlockRot b);

    class Block
    {
    public:
        /*
        Block(BlockType type, RotType x, RotType y, Display display) :
            Block(type, BlockRot{x, y}, display)
        {

        }
        Block(BlockType type, BlockRot rot, Display display) :
            type{type}, rot{rot}, display{display}
        {

        }
        Block()
        {
        }
        */

        BlockType type;
        BlockRot rot;
        Display display;

        glm::vec3 GetRotation() const;
        glm::mat4 GetRotationMat() const;
    };
    bool operator==(Block a, Block b);

    // TODO: 270 on z to 90 with Y rot
    Game::BlockRot GetNextValidRotation(Game::BlockRot baseRot, RotationAxis axis, bool positive);
    glm::vec3 BlockRotToVec3(BlockRot rot);
    Math::Transform GetBlockTransform(const Block& block, glm::ivec3 pos, float blockScale);
    Math::Transform GetBlockTransform(glm::ivec3 pos, float blockScale);
    Math::Transform GetBlockTransform(glm::ivec3 pos, Game::BlockRot rot, float blockScale);

    
}