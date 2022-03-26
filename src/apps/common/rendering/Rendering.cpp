#include <Rendering.h>

Collisions::Ray Rendering::ScreenToWorldRay(const Rendering::Camera& camera, glm::vec<2, int> screenPos, glm::vec<2, int> screenSize)
{
    glm::mat4 projViewMat = camera.GetProjViewMat();
    return Collisions::ScreenToWorldRay(projViewMat, screenPos, screenSize);
}