#include <entity/Player.h>

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


// Functions

glm::vec3 Entity::PlayerInputToMove(PlayerInput input)
{
    glm::vec3 moveDir{0.0f};
    moveDir.x = input[Entity::MOVE_RIGHT] - input[Entity::MOVE_LEFT];
    moveDir.z = input[Entity::MOVE_DOWN] - input[Entity::MOVE_UP];
    if(moveDir.x != 0.0f && moveDir.z != 0.0f)
        moveDir = glm::normalize(moveDir);  

    return moveDir;
}