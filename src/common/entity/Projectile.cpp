#include <Projectile.h>

#include <util/Random.h>

using namespace Entity;

Projectile::State Projectile::State::Interpolate(Projectile::State a, Projectile::State b, float alpha)
{
    Projectile::State ret;
    ret.pos = a.pos * alpha + b.pos * (1.0f - alpha);
    ret.rotation = a.rotation * alpha + b.rotation * (1.0f - alpha);

    return ret;
}

Projectile::State Projectile::ExtractState()
{
    Projectile::State s;

    s.pos = pos;
    s.rotation = glm::vec2{rotation.x, rotation.z};

    return s;
}

void Projectile::ApplyState(Projectile::State s)
{
    pos = s.pos;
    rotation = glm::vec3{s.rotation.x, 0.0f, s.rotation.y};
}

void Projectile::SetScale(glm::vec3 scale)
{
    this->scale = scale;
}

void Projectile::Launch(Entity::ID playerId, glm::vec3 pos, glm::vec3 iVelocity, glm::vec3 acceleration)
{
    this->playerId = playerId;
    this->pos = pos;
    this->acceleration = acceleration;
    this->velocity = iVelocity;

    auto proj = glm::normalize(glm::vec2{velocity.x, velocity.z});
    auto lapsSecs = Util::Random::Normal(0.5f, 1.5f);
    auto rotSpeed = 360.0f * lapsSecs;
    this->torque = glm::vec3{rotSpeed * proj.y, 0.0f, rotSpeed * proj.x};

    OnLaunch();
}

void Projectile::Update(Util::Time::Seconds deltaTime)
{
    float dT = deltaTime.count();
    velocity = velocity + acceleration * dT;

    auto displacement = velocity * dT;
    pos += displacement;

    rotation += torque * dT;

    OnUpdate(deltaTime);
}

// Grenade

void Grenade::OnLaunch()
{
    timer.SetDuration(timerDuration);
    timer.Start();
}

void Grenade::OnCollide(glm::vec3 surfaceNormal)
{
    velocity = glm::reflect(velocity, surfaceNormal);
    velocity = velocity * 0.66f;
}

void Grenade::OnUpdate(Util::Time::Seconds secs)
{
    timer.Update(secs);
    if(timer.IsDone())
        detonated = true;
}