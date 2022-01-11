#pragma once

#include <math/Transform.h>

namespace Entity
{
    struct PlayerHitBox
    {
        static const Math::Transform head;
        Math::Transform body;
        Math::Transform wheels;
    };

    using ID = uint8_t;
    struct Player
    {
        ID id;
        Math::Transform transform;
        bool onDmg = false;


    };
}