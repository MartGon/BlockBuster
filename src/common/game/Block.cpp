#include <Block.h>

glm::vec3 Game::Block::GetRotation() const
{
    return glm::vec3{0.0f, this->rot.y * 90.0f, this->rot.z * 90.0f};
}

Math::Transform Game::GetBlockTransform(const Block& block, glm::ivec3 pos, float blockScale)
{
    return Math::Transform{glm::vec3{pos} * blockScale, block.GetRotation(), glm::vec3{blockScale}};
}