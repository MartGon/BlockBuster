#include <Player.h>

using namespace Entity;

const Math::Transform PlayerHitBox::head{glm::vec3{0.0f, 1.5f, 0.0f}, glm::vec3{0.0f}, glm::vec3{1.5f, 1.0f, 1.5f}};
const Math::Transform PlayerHitBox::body{glm::vec3{0.0f}, glm::vec3{0.0f}, glm::vec3{2.0f}};
const Math::Transform PlayerHitBox::wheels{glm::vec3{0.0f, -1.75f, 0.0f}, glm::vec3{0.0f}, glm::vec3{3.5f,  1.25f, 2.0f}};