#include <Block.h>

bool Game::operator==(Game::Block a, Game::Block b)
{
    return (glm::ivec3)a.transform.position == (glm::ivec3)b.transform.position;
}