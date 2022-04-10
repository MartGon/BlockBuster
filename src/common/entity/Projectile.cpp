#include <Projectile.h>

using namespace Entity;

Projectile::State Projectile::State::Interpolate(Projectile::State a, Projectile::State b, float alpha)
{
    Projectile::State ret;
    ret.pos = a.pos * alpha + b.pos * (1.0f - alpha);

    return ret;
}

Projectile::State Projectile::ExtractState()
{
    Projectile::State s;

    s.pos = pos;

    return s;
}

void Projectile::ApplyState(Projectile::State s)
{
    pos = s.pos;
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

    OnLaunch();
}

void Projectile::Update(Util::Time::Seconds deltaTime)
{
    float dT = deltaTime.count();
    velocity = velocity + acceleration * dT;

    auto displacement = velocity * dT;
    pos += displacement;

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