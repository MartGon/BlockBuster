#include <Input.h>

#include <SDL2/SDL.h>

#include <debug/Debug.h>

Entity::PlayerInput Input::GetPlayerInput(Entity::PlayerInput mask)
{
    Entity::PlayerInput input;
    auto state = SDL_GetKeyboardState(nullptr);
    input[Entity::MOVE_LEFT] = state[SDL_SCANCODE_A];
    input[Entity::MOVE_RIGHT] = state[SDL_SCANCODE_D];
    input[Entity::MOVE_UP] = state[SDL_SCANCODE_W];
    input[Entity::MOVE_DOWN] = state[SDL_SCANCODE_S];

    auto mouseState = SDL_GetMouseState(nullptr, nullptr);
    input[Entity::SHOOT] = mouseState & SDL_BUTTON_LEFT;
    input[Entity::RELOAD] = state[SDL_SCANCODE_R];
    input[Entity::ACTION] = state[SDL_SCANCODE_E];

    input = input & mask;

    return input;
}

Entity::PlayerInput Input::GetPlayerInputNumpad(Entity::PlayerInput mask)
{
    Entity::PlayerInput input;
    auto state = SDL_GetKeyboardState(nullptr);
    input[Entity::MOVE_LEFT] = state[SDL_SCANCODE_KP_4];
    input[Entity::MOVE_RIGHT] = state[SDL_SCANCODE_KP_6];
    input[Entity::MOVE_UP] = state[SDL_SCANCODE_KP_8];
    input[Entity::MOVE_DOWN] = state[SDL_SCANCODE_KP_2];

    auto mouseState = SDL_GetMouseState(nullptr, nullptr);
    input[Entity::SHOOT] = mouseState & SDL_BUTTON_LEFT;
    input[Entity::RELOAD] = state[SDL_SCANCODE_R];
    input[Entity::ACTION] = state[SDL_SCANCODE_E];

    input = input & mask;

    return input;
}

Entity::PlayerInput Input::GetPlayerInputDummy(Entity::PlayerInput mask)
{
    Entity::PlayerInput input;

    input = input & mask;

    return input;
}