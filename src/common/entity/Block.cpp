#include <Block.h>

#include <entity/Map.h>

#include <math/BBMath.h>

bool Game::operator==(Display a, Display b)
{
    return a.type == b.type && a.id == b.id;
}

bool Game::operator!=(Display a, Display b)
{
    return !(a == b);
}

bool Game::operator==(BlockRot a, BlockRot b)
{
    return a.y == b.y && a.z == b.z;
}

glm::vec3 Game::Block::GetRotation() const
{
    return BlockRotToVec3(this->rot);
}

glm::mat4 Game::Block::GetRotationMat() const
{
    auto t = Game::GetBlockTransform(glm::vec3{0}, rot, 1);
    return t.GetRotationMat();    
}

bool Game::operator==(Block a, Block b)
{
    return a.display == b.display && a.type == b.type;
}

Game::BlockRot Game::GetNextValidRotation(Game::BlockRot rot, Game::RotationAxis axis, bool positive)
{
    Game::BlockRot blockRot = rot;
    auto sign = positive ? 1 : -1;
    if(axis == Game::RotationAxis::Y)
    {
        // TODO: This may cause collision issues. If that's the case, put back to ROT_180. It screws selection rotation (X/Z axis) in editor tho.
        int8_t i8rot = Math::OverflowSumInt<int8_t>(blockRot.y, sign, Game::RotType::ROT_0, Game::RotType::ROT_270);
        blockRot.y = static_cast<Game::RotType>(i8rot);
    }
    else if(axis == Game::RotationAxis::Z)
    {
        int8_t i8rot = Math::OverflowSumInt<int8_t>(blockRot.z, sign, Game::RotType::ROT_0, Game::RotType::ROT_270);
        blockRot.z = static_cast<Game::RotType>(i8rot);
    }

    return blockRot;
}

glm::vec3 Game::BlockRotToVec3(BlockRot rot)
{
    if(rot.z == RotType::ROT_270)
    {
        int8_t i8rot = Math::OverflowSumInt<int8_t>(rot.y, 1, Game::RotType::ROT_0, Game::RotType::ROT_270);
        rot.z = RotType::ROT_90;
        rot.y = static_cast<Game::RotType>(i8rot);
    }

    auto rotation = glm::vec3{0.0f, rot.y * 90.0f, rot.z * 90.0f};
    return rotation;
}

Math::Transform Game::GetBlockTransform(const Block& block, glm::ivec3 pos, float blockScale)
{
    return Math::Transform{Game::Map::ToRealPos(pos, blockScale), block.GetRotation(), glm::vec3{blockScale}};
}

Math::Transform Game::GetBlockTransform(glm::ivec3 pos, float blockScale)
{
    return Math::Transform{Game::Map::ToRealPos(pos, blockScale), glm::vec3{0.0f}, glm::vec3{blockScale}};
}

Math::Transform Game::GetBlockTransform(glm::ivec3 pos, Game::BlockRot rot, float blockScale)
{
    return Math::Transform{Game::Map::ToRealPos(pos, blockScale), BlockRotToVec3(rot), glm::vec3{blockScale}};
}
