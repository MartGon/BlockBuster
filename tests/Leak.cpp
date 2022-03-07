#include <doctest/doctest.h>

#include <game/Input.h>

#include <iostream>

// The used function is not a linear interpolation.
TEST_CASE("Get Input")
{
    auto input = Input::GetPlayerInputNumpad();

    int a = 0;
    if(input[Entity::MOVE_DOWN])
        a = 1;

    CHECK(a == 1);
}