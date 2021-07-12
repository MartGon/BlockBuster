#include <Rendering.h>

Collisions::Ray Rendering::ScreenToWorldRay(const Rendering::Camera& camera, glm::vec<2, int> screenPos, glm::vec<2, int> screenSize)
{
    glm::vec4 rayStartND{(screenPos.x * 2.0f) / (float)screenSize.x - 1.0f, 
                (screenPos.y * 2.0f) / (float) screenSize.y - 1.0f, 
                -1.0f, 1.0f};
    glm::vec4 rayEndND{(screenPos.x * 2.0f) / (float)screenSize.x - 1.0f, 
                (screenPos.y * 2.0f) / (float) screenSize.y - 1.0f, 
                0.0f, 1.0f};
        
    glm::mat4 screenToWorld = glm::inverse(camera.GetProjViewMat());
    glm::vec4 rayStartWorld = screenToWorld * rayStartND; rayStartWorld /= rayStartWorld.w;
    glm::vec4 rayEndWorld = screenToWorld * rayEndND;   rayEndWorld /= rayEndWorld.w;

    return Collisions::Ray{rayStartWorld, rayEndWorld};
}