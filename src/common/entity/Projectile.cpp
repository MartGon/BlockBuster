#include <Projectile.h>

#include <util/Random.h>

using namespace Entity;

Projectile::State Projectile::State::Interpolate(Projectile::State a, Projectile::State b, float alpha)
{
    Projectile::State ret = a;
    ret.pos = a.pos * alpha + b.pos * (1.0f - alpha);
    ret.velocity = a.velocity * alpha + b.velocity * (1.0f - alpha);
    ret.rotation = a.rotation * alpha + b.rotation * (1.0f - alpha);
    ret.type = a.type;

    return ret;
}

Projectile::State Projectile::ExtractState()
{
    Projectile::State s;

    s.pos = pos;
    s.velocity = velocity;
    s.rotation = glm::vec2{rotation.x, rotation.z};
    s.type = type;

    return s;
}

void Projectile::ApplyState(Projectile::State s)
{
    pos = s.pos;
    velocity = s.velocity;
    rotation = glm::vec3{s.rotation.x, 0.0f, s.rotation.y};
    type = s.type;
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
    travelDistance += glm::length(displacement);

    rotation += torque * dT;

    OnUpdate(deltaTime);
}

// Grenade

void Grenade::OnLaunch()
{
    timer.SetDuration(timerDuration);
    timer.Start();

    auto proj = glm::normalize(glm::vec2{velocity.x, velocity.z});
    auto lapsSecs = Util::Random::Normal(1.5f, 0.5f);
    auto rotSpeed = 360.0f * lapsSecs;
    this->torque = glm::vec3{rotSpeed * proj.y, 0.0f, rotSpeed * proj.x};
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

// Rocket

void Rocket::OnLaunch()
{
    this->acceleration = glm::vec3{0.0f};

    SetDmg(450);
    SetRadius(12.0f);

    timer.SetDuration(timerDuration);
    timer.Start();
}

void Rocket::OnUpdate(Util::Time::Seconds deltaTime)
{
    timer.Update(deltaTime);
    if(timer.IsDone())
        detonated = true;
}

void Rocket::OnCollide(glm::vec3 surfaceNormal)
{
    detonated = true;
}