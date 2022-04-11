#pragma once

#include <rendering/RenderMgr.h>
#include <entity/Projectile.h>
#include <animation/Animation.h>

#include <util/Table.h>

namespace Game::Models
{
    class ExplosionParticle
    {
    public:
        ExplosionParticle(Rendering::Billboard* expBillboard, Animation::Clip* clip, glm::vec3 pos, float rot, float scale, float startPercent);

        ExplosionParticle(const ExplosionParticle&) = delete;
        ExplosionParticle& operator=(const ExplosionParticle&) = delete;

        ExplosionParticle(ExplosionParticle&&) = default;
        ExplosionParticle& operator=(ExplosionParticle&&) = default;

        void Draw(glm::mat4 projView, glm::vec3 camRight, glm::vec3 camUp);
        void Update(Util::Time::Seconds deltaTime);
        inline bool IsDone()
        {
            return expPlayer.IsDone();
        }

    private:
        Rendering::Billboard* expBillboard;
        glm::vec3 pos;
        float rot;
        float scale;

        Animation::Player expPlayer;
        int frameId = 0;
        bool isDone = false;
    };

    class Explosion
    {
    friend class ExplosionMgr;
    public:
        Explosion(const Explosion&) = delete;
        Explosion& operator=(const Explosion&) = delete;

        Explosion(Explosion&&) = default;
        Explosion& operator=(Explosion&&) = default;

        static constexpr float CENTER_OFFSET_MEAN = 0.0f;
        static constexpr float CENTER_OFFSET_SD = 0.5f;
        static constexpr int PARTICLE_MEAN = 10;
        static constexpr int PARTICLE_SD = 4;

        void Draw(glm::mat4 projView, glm::vec3 camRight, glm::vec3 camUp);
        void Update(Util::Time::Seconds deltaTime);
        bool IsOver();

    private:
        Explosion(Rendering::Billboard* explosionBillboard, Animation::Clip* clip, glm::vec3 center);
        std::vector<ExplosionParticle> particles;
    };

    class ExplosionMgr
    {
    public:
        void Start(Rendering::RenderMgr& renderMgr, GL::Shader& expShader);

        void CreateExplosion(glm::vec3 center);
        void Update(Util::Time::Seconds deltaTime);

        void DrawExplosions(glm::mat4 projView, glm::vec3 camRight, glm::vec3 camUp);

    private:
        Rendering::Billboard* expBillboard;
        Animation::Clip frameInc;
        Rendering::TextureID expTexId;
        Util::Table<Explosion> explosions;
    };
}