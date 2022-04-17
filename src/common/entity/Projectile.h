#pragma once

#include <math/Transform.h>
#include <util/BBTime.h>
#include <util/Timer.h>

namespace Entity
{
    using ID = uint8_t;
    class Projectile
    {
    public:
        enum Type : uint8_t
        {
            NONE,
            GRENADE,
            ROCKET,
            COUNT
        };

        struct State
        {
            glm::vec3 pos;
            glm::vec2 rotation;
            glm::vec3 velocity;
            Type type;

            static State Interpolate(State a, State b, float alpha);
        };

        virtual ~Projectile() = default;
        Projectile(Type type) : type{type}
        {

        }

        virtual void OnLaunch(){};
        virtual void OnCollide(glm::vec3 surfaceNormal){};
        virtual void OnUpdate(Util::Time::Seconds deltaTime) = 0;

        State ExtractState();
        void ApplyState(State state);

        void Launch(Entity::ID playerId, glm::vec3 pos, glm::vec3 iVelocity, glm::vec3 acceleration);
        void Update(Util::Time::Seconds deltaTime); 


        inline glm::vec3 GetPos() const
        {
            return pos;
        }

        inline void SetPos(glm::vec3 pos)
        {
            this->pos = pos;
        }

        inline glm::vec3 GetVelocity() const
        {
            return this->velocity;
        }

        inline glm::vec3 GetRotation() const
        {
            return rotation;
        }

        inline glm::vec3 GetScale() const
        {
            return scale;
        }

        inline void SetScale(glm::vec3 scale)
        {
            this->scale = scale;
        }

        inline float GetRadius() const
        {
            return radius;
        }

        inline void SetRadius(float radius)
        {
            this->radius = radius;
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

        inline Type GetType()
        {
            return type;
        }

    protected:
        glm::vec3 scale{1.0f};
        glm::vec3 pos{0.0f};
        glm::vec3 velocity{0.0f};
        glm::vec3 acceleration{0.0f};

        glm::vec3 torque{0.0f};
        glm::vec3 rotation{0.0f};

        Entity::ID playerId;
        float radius = 8.0f;
        float dmg = 375.f;

        Type type;
        bool detonated = false;
    };

    class Grenade : public Projectile
    {
    public:
        Grenade() : Projectile(Type::GRENADE)
        {
        }

        void OnLaunch() override;
        void OnUpdate(Util::Time::Seconds secs) override;
        void OnCollide(glm::vec3 surfaceNormal) override;

    private:
        constexpr static Util::Time::Seconds timerDuration{3.0f};
        Util::Timer timer;
    };

    class Rocket : public Projectile
    {
    public:
        Rocket() : Projectile(Type::ROCKET)
        {
            SetScale(glm::vec3{0.15f});
        }

        void OnLaunch() override;
        void OnUpdate(Util::Time::Seconds secs) override;
        void OnCollide(glm::vec3 surfaceNormal) override;

    private:
        constexpr static Util::Time::Seconds timerDuration{15.0f};
        Util::Timer timer;
    };
}