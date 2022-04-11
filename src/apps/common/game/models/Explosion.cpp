#include <models/Explosion.h>

#include <util/Random.h>

using namespace Game::Models;

ExplosionParticle::ExplosionParticle(Rendering::Billboard* expBillboard, Animation::Clip* clip, glm::vec3 pos, float rot, float scale, float startPercent) : 
    expBillboard{expBillboard}, pos{pos}, rot{rot}, scale{scale}
{
    frameId = 36 * startPercent;

    expPlayer.SetClip(clip);
    expPlayer.SetTargetInt("frame", &frameId);
    expPlayer.Play();
    expPlayer.Update(Util::Time::Seconds{2.0f} * startPercent);
}

void ExplosionParticle::Draw(glm::mat4 projView, glm::vec3 camRight, glm::vec3 camUp)
{
    if(!isDone)
        expBillboard->Draw(projView, pos, camRight, camUp, rot, glm::vec2{scale}, glm::vec4{1.0f}, Rendering::RenderMgr::NO_FACE_CULLING, frameId);
}

void ExplosionParticle::Update(Util::Time::Seconds deltaTime)
{
    expPlayer.Update(deltaTime);
}

// Explosion

Explosion::Explosion(Rendering::Billboard* billboard, Animation::Clip* clip, glm::vec3 center)
{
    int particleNum = std::max(Util::Random::Normal<float>(PARTICLE_MEAN, PARTICLE_SD), 6.0f);
    particles.reserve(particleNum);
    for(auto i = 0; i < particleNum; i++)
    {
        auto x = Util::Random::Normal<float>(CENTER_OFFSET_MEAN, CENTER_OFFSET_SD);
        auto y = Util::Random::Normal<float>(CENTER_OFFSET_MEAN, CENTER_OFFSET_SD);
        auto z = Util::Random::Normal<float>(CENTER_OFFSET_MEAN, CENTER_OFFSET_SD);
        auto expCenter = center + glm::vec3{x, y, z};

        auto rot = Util::Random::Uniform(0, glm::two_pi<float>());
        auto startPercent = Util::Random::Uniform(0.0f, 0.25f);
        auto scale = Util::Random::Normal(4.0f, 2.0f);
        
        particles.emplace_back(billboard, clip, expCenter, rot, scale, startPercent);
    }
}

void Explosion::Draw(glm::mat4 projView, glm::vec3 camRight, glm::vec3 camUp)
{
    for(auto& particle : particles)
        particle.Draw(projView, camRight, camUp);
}

void Explosion::Update(Util::Time::Seconds secs)
{
    for(auto& particle : particles)
        particle.Update(secs);
}

bool Explosion::IsOver()
{
    bool isOver = true;
    for(auto& particle : particles)
        if(!particle.IsDone())
            return false;

    return isOver;
}

// Explosion Mgr

void ExplosionMgr::Start(Rendering::RenderMgr& renderMgr, GL::Shader& expShader)
{
    // Texture
    auto& texMgr = renderMgr.GetTextureMgr();
    expTexId = texMgr.LoadFromDefaultFolder("explosion.png", true);
    
    // Billboard
    expBillboard = renderMgr.CreateBillboard();
    expBillboard->painting = Rendering::Painting{.type = Rendering::PaintingType::TEXTURE, .hasAlpha = true, .texture = expTexId};
    expBillboard->shader = &expShader;

    // Animation
    Animation::Sample s1{
        {},
        {},
        {{"frame", 0}}
    };
    Animation::KeyFrame f1{s1, 0};    
    Animation::Sample s2{
        {},
        {},
        {{"frame", 36}}
    };
    Animation::KeyFrame f2{s2, 120};
    frameInc.keyFrames = {f1, f2};
    frameInc.fps = 60.0f;
}

void ExplosionMgr::CreateExplosion(glm::vec3 center)
{
    Explosion explosion(expBillboard, &frameInc, center);
    explosions.MoveInto(std::move(explosion));
}

void ExplosionMgr::Update(Util::Time::Seconds deltaTime)
{
    std::vector<uint16_t> toRemove;
    for(auto id : explosions.GetIDs())
    {
        auto& explosion = explosions.GetRef(id);
        explosion.Update(deltaTime);
        if(explosion.IsOver())
            toRemove.push_back(id);
    }

    for(auto id : toRemove)
        explosions.Remove(id);
}

void ExplosionMgr::DrawExplosions(glm::mat4 projView, glm::vec3 camRight, glm::vec3 camUp)
{
    for(auto id : explosions.GetIDs())
    {
        auto& explosion = explosions.GetRef(id);
        explosion.Draw(projView, camRight, camUp);
    }
}