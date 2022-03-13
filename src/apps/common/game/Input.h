#pragma once

#include <entity/Player.h>

namespace Input
{
    Entity::PlayerInput GetPlayerInput(Entity::PlayerInput mask);
    Entity::PlayerInput GetPlayerInputNumpad(Entity::PlayerInput mask);
    Entity::PlayerInput GetPlayerInputDummy(Entity::PlayerInput mask);
}