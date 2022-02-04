#pragma once

#include <math/Transform.h>
#include <entity/Weapon.h>

namespace Entity
{
    struct PlayerHitBox
    {
        static const Math::Transform head;
        static const Math::Transform body;
        static const Math::Transform wheels; // This one should rotate with the wheels, so it's affected by moveDir
    };

    struct PlayerState
    {
        glm::vec3 pos;
        glm::vec2 rot;
        bool onDmg;
    };

    enum Inputs
    {
        MOVE_DOWN,
        MOVE_UP,        
        MOVE_LEFT,
        MOVE_RIGHT,
        SHOT,

        MAX
    };

    struct PlayerInput
    {
        bool inputs[Inputs::MAX];

        bool& operator[](uint32_t index)
        {
            return inputs[index];
        }
    };

    glm::vec3 PlayerInputToMove(PlayerInput input);

    using ID = uint8_t;
    struct Player
    {
        static const Math::Transform moveCollisionBox; // Only affects collision with terrain

        ID id;
        Math::Transform transform;
        bool onDmg = false;

            // State
        static const float MAX_SHIELD;
        static const float MAX_HEALTH;
        float shield = MAX_SHIELD;
        float health = MAX_HEALTH;

        glm::vec3 lastMoveDir;

        Weapon* weapon = nullptr;

        bool IsHitByRay();
    };
}