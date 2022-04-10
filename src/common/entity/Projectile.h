#pragma once

#include <math/Transform.h>
#include <util/BBTime.h>
#include <util/Timer.h>
#include <entity/Player.h>

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

        virtual void OnLaunch(){};
        virtual void OnCollide(glm::vec3 surfaceNormal){};
        virtual void OnUpdate(Util::Time::Seconds deltaTime){};

        State ExtractState();
        void ApplyState(State state);

        void Launch(Entity::ID playerId, glm::vec3 pos, glm::vec3 iVelocity, glm::vec3 acceleration);
        void Update(Util::Time::Seconds deltaTime); 

        void SetScale(glm::vec3 scale);
        void SetRadius(float radius);

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

        inline float GetRadius() const
        {
            return radius;
        }

        inline float GetDmg() const
        {
            return dmg;
        }

        inline void SetDmg(float dmg)
        {
            this->dmg = dmg;
        }

        inline Entity::ID GetAuthor() const
        {
            return playerId;
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

        Entity::ID playerId;
        float radius = 8.0f;
        float dmg = 375.f;

        bool detonated = false;
    };

    class Grenade : public Projectile
    {
    public:
        void OnLaunch() override;
        void OnUpdate(Util::Time::Seconds secs) override;
        void OnCollide(glm::vec3 surfaceNormal) override;

    private:
        constexpr static Util::Time::Seconds timerDuration{3.0f};
        Util::Timer timer;
    };
}