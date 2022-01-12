#pragma once

#include <math/Transform.h>

namespace Entity
{
    struct PlayerHitBox
    {
        static const Math::Transform head;
        static const Math::Transform body;
        static const Math::Transform wheels; // This one should rotate with the wheels
    };

    using ID = uint8_t;
    struct Player
    {
        ID id;
        Math::Transform transform;
        bool onDmg = false;

        glm::vec3 lastMoveDir;

        bool IsHitByRay();
    };
}