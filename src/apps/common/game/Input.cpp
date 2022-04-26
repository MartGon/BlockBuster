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
    input[Entity::JUMP] = state[SDL_SCANCODE_SPACE];

    auto mouseState = SDL_GetMouseState(nullptr, nullptr);
    input[Entity::SHOOT] = mouseState & SDL_BUTTON_LMASK || state[SDL_SCANCODE_G];
    input[Entity::ALT_SHOOT] = mouseState & SDL_BUTTON_RMASK;
    input[Entity::RELOAD] = state[SDL_SCANCODE_R];
    input[Entity::ACTION] = state[SDL_SCANCODE_E];
    input[Entity::GRENADE] = state[SDL_SCANCODE_F];
    input[Entity::WEAPON_SWAP_0] = state[SDL_SCANCODE_Q];
    
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
    input[Entity::JUMP] = state[SDL_SCANCODE_SPACE];

    auto mouseState = SDL_GetMouseState(nullptr, nullptr);
    input[Entity::SHOOT] = mouseState & SDL_BUTTON_LMASK;
    input[Entity::ALT_SHOOT] = mouseState & SDL_BUTTON_RMASK;
    input[Entity::RELOAD] = state[SDL_SCANCODE_R];
    input[Entity::ACTION] = state[SDL_SCANCODE_E];
    input[Entity::GRENADE] = state[SDL_SCANCODE_F];
    input[Entity::WEAPON_SWAP_0] = state[SDL_SCANCODE_Q];

    input = input & mask;

    return input;
}

Entity::PlayerInput Input::GetPlayerInputDummy(Entity::PlayerInput mask)
{
    Entity::PlayerInput input;

    input = input & mask;

    return input;
}