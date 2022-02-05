#pragma once

#include <math/Transform.h>

#include <entity/Block.h>
#include <entity/Map.h>
#include <entity/Player.h>

#include <vector>

namespace Entity
{
    class PlayerController
    {
    public:

        void Update(Entity::PlayerInput input);
        void HandleCollisions(Game::Map::Map* map, float blockScale);
        void HandleCollisions(const std::vector<std::pair<Math::Transform, Game::Block>> &blocks);
        
        Math::Transform transform;

        float speed = 0.1f;
        float height = 2.0f;
        float gravitySpeed = -0.4f;

        bool gravity = false;

    private:
        glm::vec3 prevPos;
        
        bool isOnSlope = false;
        bool wasOnSlope = false;
        glm::vec3 slopeNormal;
        Math::Transform slopeTransform;
    };

}