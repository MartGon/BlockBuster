#include <glm/glm.hpp>


namespace Collisions
{

    struct RayIntersection
    {
        bool intersects;
        glm::vec2 ts;
        glm::vec3 normal;
    };

    RayIntersection RayAABBIntersection(glm::vec3 rayOrigin, glm::vec3 rayDir, glm::vec3 boxSize);
    RayIntersection RaySlopeIntersection(glm::vec3 rayOrigin, glm::vec3 rayDir, glm::vec3 boxSize);

    struct AABBIntersection
    {
        bool intersects;
        glm::vec3 offset;
        glm::vec3 min;
        glm::vec3 normal;
    };

    AABBIntersection AABBCollision(glm::vec3 posA, glm::vec3 sizeA, glm::vec3 posB, glm::vec3 sizeB);
    AABBIntersection AABBSlopeCollision(glm::vec3 posA, glm::vec3 sizeA, glm::vec3 sizeB, float precision = 0.005f);

};