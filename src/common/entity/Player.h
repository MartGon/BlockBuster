#pragma once

#include <math/Transform.h>

namespace Entity
{
    using ID = uint8_t;
    struct Player
    {
        ID id;
        Math::Transform transform;
        bool onDmg = false;
    };
}