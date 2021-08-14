#include <Block.h>

bool Game::operator==(Game::Block a, Game::Block b)
{
    return (glm::ivec3)a.transform.position == (glm::ivec3)b.transform.position;
}

glm::vec3 Game::Block::GetRotation() const
{
    return glm::vec3{0.0f, this->rot.y * 90.0f, this->rot.z * 90.0f};
}