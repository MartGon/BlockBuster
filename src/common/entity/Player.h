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

    enum Inputs
    {
        MOVE_DOWN,
        MOVE_UP,        
        MOVE_LEFT,
        MOVE_RIGHT,
        SHOOT,

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

    struct PlayerState
    {
        glm::vec3 pos;
        glm::vec2 rot;
        bool onDmg;
    };

    void Interpoalte(PlayerState a, PlayerState b, float alpha);

    using ID = uint8_t;
    struct Player
    {
        static const Math::Transform moveCollisionBox; // Only affects collision with terrain


        void ApplyState(PlayerState state);
        PlayerState ExtractState();

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