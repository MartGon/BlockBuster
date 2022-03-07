#pragma once

#include <entity/Player.h>

namespace Input
{
    Entity::PlayerInput GetPlayerInput();
    Entity::PlayerInput GetPlayerInputNumpad();
    Entity::PlayerInput GetPlayerInputDummy();
}