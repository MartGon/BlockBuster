#pragma once

#include <math/Transform.h>

#include <entity/Block.h>
#include <entity/Map.h>
#include <entity/Player.h>

#include <util/Timer.h>

#include <vector>
#include <optional>

namespace Entity
{
    class PlayerController
    {
    public:

        Entity::PlayerState UpdatePosition(Entity::PlayerState ps, Entity::PlayerInput input, Game::Map::Map* map, Util::Time::Seconds deltaTime);
        Weapon UpdateWeapon(Weapon weapon, Weapon secWeapon, Entity::PlayerInput input, Util::Time::Seconds deltaTime);
        Player::HealthState UpdateShield(Player::HealthState healthState, Util::Timer& dmgTimer, Util::Time::Seconds deltaTime);

        Math::Transform GetECB();
        Math::Transform GetGCB();

        float speed = 8.0f;
        float gravityAcceleration = -25.0f;

        float terminalFallSpeed = -30.0f;
        float jumpSpeed = 10.0f; // Slightly higher than a block
        constexpr static float fallSpeed = -12.0f; 
    private:

        Math::Transform transform;

        glm::vec3 HandleCollisions(Game::Map::Map* map, const std::vector<glm::ivec3> &blocks, glm::vec3 normalMask = glm::vec3{1.f});
        glm::vec3 HandleGravityCollisions(Game::Map::Map* map, const std::vector<glm::ivec3> &blocks);
        std::vector<glm::ivec3> GetCollisionBlocks(Game::Map::Map* map, float blockScale);

    #ifdef ALT_COLLISIONS
        glm::vec3 HandleGravityCollisionsAlt(Game::Map::Map* map, float blockScale, Util::Time::Seconds deltaTime);
        std::optional<glm::vec3> HandleGravityCollisionAltBlock(Game::Map::Map* map, glm::vec3 bottomPoint, glm::ivec3 blockIndex, glm::vec3 offset);
    #endif
    };

}