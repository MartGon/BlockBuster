#pragma once

#include <math/Transform.h>

#include <entity/Block.h>
#include <entity/Map.h>
#include <entity/Player.h>

#include <util/Timer.h>

#include <vector>

namespace Entity
{
    class PlayerController
    {
    public:

        void Update(Entity::PlayerInput input, Game::Map::Map* map, Util::Time::Seconds deltaTime);
        
        Math::Transform transform;

        Math::Transform GetECB();
        Math::Transform GetGCB();

        float speed = 5.f;
        float height = 2.0f;
        float gravitySpeed = -0.4f;
    private:

        glm::vec3 HandleCollisions(Game::Map::Map* map, const std::vector<glm::ivec3> &blocks, glm::vec3 normalMask = glm::vec3{1.f});
        glm::vec3 HandleGravityCollisions(Game::Map::Map* map, const std::vector<glm::ivec3> &blocks);
        std::vector<glm::ivec3> GetCollisionBlocks(Game::Map::Map* map, float blockScale);

        glm::vec3 HandleGravityCollisionsAlt(Game::Map::Map* map, float blockScale);
        std::optional<glm::vec3> HandleGravityCollisionAltBlock(Game::Map::Map* map, glm::vec3 bottomPoint, glm::ivec3 blockIndex, glm::vec3 offset);
    };

}