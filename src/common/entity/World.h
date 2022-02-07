#pragma once

#include <entity/Map.h>

namespace Entity
{
    struct World
    {
        float blockScale;
        Game::Map::Map map;
    };
}