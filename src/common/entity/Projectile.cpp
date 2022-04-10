#include <Projectile.h>

#include <util/Random.h>

using namespace Entity;

Projectile::State Projectile::State::Interpolate(Projectile::State a, Projectile::State b, float alpha)
{
    Projectile::State ret;
    ret.pos = a.pos * alpha + b.pos * (1.0f - alpha);
    ret.pitch = a.pitch * alpha + b.pitch * (1.0f - alpha);

    return ret;
}

Projectile::State Projectile::ExtractState()
{
    Projectile::State s;

    s.pos = pos;
    s.pitch = rotation.x;

    return s;
}

void Projectile::ApplyState(Projectile::State s)
{
    pos = s.pos;
    rotation = glm::vec3{s.pitch, 0.0f, 0.0f};
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
    auto lapsSecs = Util::Random::Normal(0.0f, 2.0f);
    this->torque = glm::vec3{360.0f * lapsSecs, 0.0f, 0.0f};

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