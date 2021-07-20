#pragma once

#include <SDL2/SDL.h>

#include <math/Transform.h>

#include <game/Block.h>

#include <vector>

namespace AppGame
{
    class Player
    {
    public:

        void Update();
        void HandleSDLEvent(const SDL_Event& event);
        void HandleCollisions(std::vector<Game::Block> blocks);
        
        Math::Transform transform;

        float speed = 0.05f;
        float gravitySpeed = -0.4f;

    private:
        glm::vec3 prevPos;
        bool gravity = false;
    };

}