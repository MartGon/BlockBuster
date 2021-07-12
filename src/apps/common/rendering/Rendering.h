#pragma once

#include <Camera.h>
#include <collisions/Collisions.h>

namespace Rendering
{
    Collisions::Ray ScreenToWorldRay(const Rendering::Camera& camera, glm::vec<2, int> screenPos, glm::vec<2, int> screenSize);
}