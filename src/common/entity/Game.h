
#include <entity/Block.h>
#include <entity/Map.h>
#include <Player.h>
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

    struct RayPlayerCollision
    {
        bool collides = false;
        Entity::Player::HitBoxType hitboxType;
        Collisions::RayIntersection intersection;
    };
    RayPlayerCollision RayCollidesWithPlayer(Collisions::Ray ray, glm::vec3 playerPos, float playerYaw, glm::vec3 lastMoveDir);
    RayPlayerCollision RayCollidesWithPlayerHitbox(Collisions::Ray ray, glm::vec3 playerPos, float playerYaw, Entity::Player::HitBoxType type);

    Collisions::Intersection AABBCollidesBlock(Game::Map::Map* map, Math::Transform aabb);
}