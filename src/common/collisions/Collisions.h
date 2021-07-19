#pragma once

#include <glm/glm.hpp>

#include <math/Transform.h>

namespace Collisions
{
    struct Ray
    {
        glm::vec3 origin;
        glm::vec3 dest;

        glm::vec3 GetDir() const;
    };

    struct RayIntersection
    {
        bool intersects;
        glm::vec2 ts;
        glm::vec3 normal;
    };

    RayIntersection RayAABBIntersection(Ray modelRay, glm::vec3 boxSize);
    RayIntersection RayAABBIntersection(Ray worldRay, glm::mat4 modelMat);
    RayIntersection RaySlopeIntersection(Ray modelRay, glm::vec3 boxSize);
    RayIntersection RaySlopeIntersection(Ray worldRay, glm::mat4 modelMat);
    Ray ToModelSpace(Ray ray, glm::mat4 modelMat);
    glm::vec3 ToWorldSpace(glm::vec3 normal, glm::mat4 modelMat);

    struct AABBIntersection
    {
        bool collides;
        bool intersects;
        glm::vec3 offset;
        glm::vec3 normal;
    };

    AABBIntersection AABBCollision(glm::vec3 posA, glm::vec3 sizeA, glm::vec3 posB, glm::vec3 sizeB);

    struct AABBSlopeIntersection
    {
        bool collides;
        bool intersects;
        glm::vec3 offset;
        glm::vec3 normal;

        
        glm::vec3 min;
        glm::vec3 minAxis;
        glm::vec3 sign;
    };

    AABBSlopeIntersection AABBSlopeCollision(glm::vec3 posA, glm::vec3 prevPosA, glm::vec3 sizeA, glm::vec3 sizeB, float precision = 0.005f);
    AABBSlopeIntersection AABBSlopeCollision(Math::Transform transformAABB, Math::Transform prevTransformAABB, Math::Transform transformSlope, float precision = 0.005f);

};