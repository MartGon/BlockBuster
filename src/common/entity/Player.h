#pragma once

#include <math/Transform.h>
#include <Weapon.h>

namespace Entity
{
    struct PlayerHitBox
    {
        static const Math::Transform head;
        static const Math::Transform body;
        static const Math::Transform wheels; // This one should rotate with the wheels, so it's affected by moveDir
    };

    using ID = uint8_t;
    struct Player
    {
        static const Math::Transform moveCollisionBox; // Only affects collision with terrain

        ID id;
        Math::Transform transform;
        bool onDmg = false;

        glm::vec3 lastMoveDir;

        Weapon* weapon = nullptr;

        bool IsHitByRay();
    };
}