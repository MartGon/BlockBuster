#include <entity/Player.h>
#include <math/Interpolation.h>

using namespace Entity;

// Player Hit boxes
const Math::Transform PlayerHitBox::head{glm::vec3{0.0f, 1.5f, 0.0f}, glm::vec3{0.0f}, glm::vec3{1.5f, 1.0f, 1.5f}};
const Math::Transform PlayerHitBox::body{glm::vec3{0.0f}, glm::vec3{0.0f}, glm::vec3{2.0f}};
const Math::Transform PlayerHitBox::wheels{glm::vec3{0.0f, -1.75f, 0.0f}, glm::vec3{0.0f}, glm::vec3{3.5f,  1.25f, 2.0f}};

// Player move collision boxes
const Math::Transform Player::moveCollisionBox{glm::vec3{0.0f, -0.25f, 0.0f}, glm::vec3{0.0f}, glm::vec3{2.0f, 4.5f, 2.0f}};

// Player stats

const float Player::MAX_HEALTH = 100.0f;
const float Player::MAX_SHIELD = 300.0f;

// Player State

Math::Transform PlayerState::GetTransform()
{
    Math::Transform t{pos, glm::vec3{rot, 0.0f}, glm::vec3{1.0f}};
    return t;
}

// Functions

// Note/TODO: Commented code in operators are in place due to wierd behaviour with prediction error correction
Entity::PlayerState Entity::operator+(const Entity::PlayerState& a, const Entity::PlayerState& b)
{
    Entity::PlayerState res = a;
    res.pos = a.pos + b.pos;
    //res.rot = a.rot + b.rot;

    return res;
}

Entity::PlayerState Entity::operator-(const Entity::PlayerState& a, const Entity::PlayerState& b)
{
    Entity::PlayerState res = a;
    res.pos = a.pos - b.pos;
    //res.rot = a.rot - b.rot;

    return res;
}

Entity::PlayerState Entity::operator*(const Entity::PlayerState& a, float b)
{
    Entity::PlayerState res = a;
    res.pos = a.pos * b;
    //res.rot = a.rot * b;

    return res;
}

glm::vec3 Entity::PlayerInputToMove(PlayerInput input)
{
    glm::vec3 moveDir{0.0f};
    moveDir.x = input[Entity::MOVE_RIGHT] - input[Entity::MOVE_LEFT];
    moveDir.z = input[Entity::MOVE_DOWN] - input[Entity::MOVE_UP];
    if(moveDir.x != 0.0f && moveDir.z != 0.0f)
        moveDir = glm::normalize(moveDir);  

    return moveDir;
}

float Entity::GetDifference(PlayerState a, PlayerState b)
{
    auto distance = glm::length(a.pos - b.pos);
    auto diff = distance / glm::length(a.pos);

    return diff;
}

Entity::PlayerState Entity::Interpolate(Entity::PlayerState a, Entity::PlayerState b, float alpha)
{
    Entity::PlayerState res;

    auto pos1 = a.pos;
    auto pos2 = b.pos;
    res.pos = pos1 * alpha + pos2 * (1 - alpha);

    auto pitch1 = a.rot.x;
    auto pitch2 = b.rot.x;
    res.rot.x = Math::InterpolateDeg(pitch1, pitch2, alpha);

    auto yaw1 = a.rot.y;
    auto yaw2 = b.rot.y;
    res.rot.y = Math::InterpolateDeg(yaw1, yaw2, alpha);

    return res;
}

// Player

Entity::PlayerState Player::ExtractState() const
{
    Entity::PlayerState s;

    s.pos = this->transform.position;
    s.rot = glm::vec2{transform.rotation};

    return s;
}

void Player::ApplyState(Entity::PlayerState s)
{
    this->transform.position = s.pos;
    this->transform.rotation = glm::vec3{s.rot, 0.0f};
}