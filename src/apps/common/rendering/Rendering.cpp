#include <Rendering.h>

Collisions::Ray Rendering::ScreenToWorldRay(const Rendering::Camera& camera, glm::vec<2, int> screenPos, glm::vec<2, int> screenSize)
{
    glm::mat4 projViewMat = camera.GetProjViewMat();
    return Collisions::ScreenToWorldRay(projViewMat, screenPos, screenSize);
}

glm::u8vec4 Rendering::FloatColorToUint8(glm::vec4 color)
{
    return glm::u8vec4{color * 255.f};
}

glm::vec4 Rendering::Uint8ColorToFloat(glm::u8vec4 color)
{
    return glm::vec4{color} / 255.f;
}