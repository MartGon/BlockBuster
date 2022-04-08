#pragma once

#include <math/Transform.h>
#include <util/BBTime.h>

namespace Entity
{
    class Projectile
    {
    public:

        struct State
        {
            glm::vec3 pos;

            static State Interpolate(State a, State b, float alpha);
        };

        virtual void OnCollide(glm::vec3 surfaceNormal){};

        State ExtractState();
        void ApplyState(State state);

        void SetScale(glm::vec3 scale);
        void Launch(glm::vec3 pos, glm::vec3 iVelocity, glm::vec3 acceleration);
        void Update(Util::Time::Seconds deltaTime); 

        inline glm::vec3 GetPos() const
        {
            return pos;
        }

        inline void SetPos(glm::vec3 pos)
        {
            this->pos = pos;
        }

        inline glm::vec3 GetScale() const
        {
            return scale;
        }

        inline bool HasDenotaded()
        {
            return detonated;
        }

    protected:
        glm::vec3 scale{1.0f};
        glm::vec3 pos{0.0f};
        glm::vec3 velocity{0.0f};
        glm::vec3 acceleration{0.0f};

        bool detonated = false;
    };

    class Grenade : public Projectile
    {
    public:

        void OnCollide(glm::vec3 surfaceNormal) override;
    };
}