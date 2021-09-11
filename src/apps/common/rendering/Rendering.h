#pragma once

#include <Camera.h>
#include <collisions/Collisions.h>

namespace Rendering
{
    Collisions::Ray ScreenToWorldRay(const Rendering::Camera& camera, glm::vec<2, int> screenPos, glm::vec<2, int> screenSize);

    glm::u8vec4 FloatColorToUint8(glm::vec4 color);
    glm::vec4 Uint8ColorToFloat(glm::u8vec4 color);
}