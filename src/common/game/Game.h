
#include <Block.h>
#include <Map.h>
#include <collisions/Collisions.h>

namespace Game
{
    struct RayBlockIntersection
    {
        glm::ivec3 pos;
        Game::Block const* block;
        Collisions::RayIntersection intersection;
    };

    std::vector<RayBlockIntersection> CastRay(Game::Map::Map* map, Collisions::Ray ray, float blockScale);
    RayBlockIntersection CastRayFirst(Game::Map::Map* map, Collisions::Ray ray, float blockScale);
}