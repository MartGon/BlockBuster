#pragma once

#include <math/Transform.h>

namespace Entity
{
    class Projectile
    {
    public:

        struct State
        {
            Math::Transform transform;
        };

        virtual void OnCollide(glm::vec3 surfaceNormal){};

    private:
        State state;
        glm::vec3 velocity;
        glm::vec3 aceleration;
    };
}