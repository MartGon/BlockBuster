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
        glm::vec3 HandleCollisions(Game::Map::Map* map, float blockScale, bool gravity);
        glm::vec3 HandleCollisions(const std::vector<std::pair<Math::Transform, Game::Block>> &blocks, bool gravity);
        
        Math::Transform transform;

        float speed = 5.f;
        float height = 2.0f;
        float gravitySpeed = -0.4f;
    private:
        glm::vec3 HandleGravityCollisions(Game::Map::Map* map, float blockScale);
        Math::Transform GetECB();
    };

}