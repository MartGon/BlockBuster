#include <Block.h>

glm::vec3 Game::Block::GetRotation() const
{
    return BlockRotToVec3(this->rot);
}

glm::vec3 Game::BlockRotToVec3(BlockRot rot)
{
    return glm::vec3{0.0f, rot.y * 90.0f, rot.z * 90.0f};
}

Math::Transform Game::GetBlockTransform(const Block& block, glm::ivec3 pos, float blockScale)
{
    return Math::Transform{glm::vec3{pos} * blockScale, block.GetRotation(), glm::vec3{blockScale}};
}

Math::Transform Game::GetBlockTransform(glm::ivec3 pos, float blockScale)
{
    return Math::Transform{glm::vec3{pos} * blockScale, glm::vec3{0.0f}, glm::vec3{blockScale}};
}

Math::Transform Game::GetBlockTransform(glm::ivec3 pos, Game::BlockRot rot, float blockScale)
{
    return Math::Transform{glm::vec3{pos} * blockScale, BlockRotToVec3(rot), glm::vec3{blockScale}};
}