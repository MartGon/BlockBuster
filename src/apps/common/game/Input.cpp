#include <Input.h>

#include <SDL2/SDL.h>

Entity::PlayerInput Input::GetPlayerInput()
{
    Entity::PlayerInput input;
    auto state = SDL_GetKeyboardState(nullptr);
    input[Entity::MOVE_LEFT] = state[SDL_SCANCODE_A];
    input[Entity::MOVE_RIGHT] = state[SDL_SCANCODE_D];
    input[Entity::MOVE_UP] = state[SDL_SCANCODE_W];
    input[Entity::MOVE_DOWN] = state[SDL_SCANCODE_S];


    auto mouseState = SDL_GetMouseState(nullptr, nullptr);
    input[Entity::SHOT] = mouseState & SDL_BUTTON_LEFT;

    return input;
}

Entity::PlayerInput Input::GetPlayerInputNumpad()
{
    Entity::PlayerInput input;
    auto state = SDL_GetKeyboardState(nullptr);
    input[Entity::MOVE_LEFT] = state[SDL_SCANCODE_KP_4];
    input[Entity::MOVE_RIGHT] = state[SDL_SCANCODE_KP_6];
    input[Entity::MOVE_UP] = state[SDL_SCANCODE_KP_8];
    input[Entity::MOVE_DOWN] = state[SDL_SCANCODE_KP_2];


    auto mouseState = SDL_GetMouseState(nullptr, nullptr);
    input[Entity::SHOT] = mouseState & SDL_BUTTON_LEFT;

    return input;
}