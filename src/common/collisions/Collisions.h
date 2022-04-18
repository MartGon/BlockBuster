#pragma once

#include <glm/glm.hpp>

#include <math/Transform.h>
#include <math/BBMath.h>

namespace Collisions
{
    // Rays
    struct Ray
    {
        glm::vec3 origin;
        glm::vec3 dest;

        glm::vec3 GetDir() const;
    };
    Collisions::Ray ScreenToWorldRay(const glm::mat4& projViewmat, glm::vec2 screenPos, glm::vec2 screenSize);

    struct RayIntersection
    {
        bool intersects = false;
        glm::vec2 ts;
        glm::vec3 normal;
        glm::vec3 colPoint;

        float GetRayLength(Ray ray) const;
    };

    RayIntersection RayAABBIntersection(Ray modelRay, glm::vec3 boxSize);
    RayIntersection RayAABBIntersection(Ray worldRay, glm::mat4 modelMat);
    RayIntersection RaySlopeIntersection(Ray modelRay, glm::vec3 boxSize);
    RayIntersection RaySlopeIntersection(Ray worldRay, glm::mat4 modelMat);
    Ray ToModelSpace(Ray ray, glm::mat4 modelMat);
    glm::vec3 ToWorldSpace(glm::vec3 vec, glm::mat4 modelMat);
    glm::vec3 NormalToWorldSpace(glm::vec3 normal, glm::mat4 modelMat);

    // General
    struct Intersection
    {
        bool collides;
        glm::vec3 normal;
        glm::vec3 offset;
    };

    // AABB
    struct AABBIntersection
    {
        bool collides;
        bool intersects;
        glm::vec3 offset;
        glm::vec3 normal;

        operator bool() const
        {
            return collides;
        }
    };

    AABBIntersection AABBCollision(glm::vec3 posA, glm::vec3 sizeA, glm::vec3 posB, glm::vec3 sizeB);


    // AABB - Slope
    struct AABBSlopeIntersection
    {
        bool collides;
        bool intersects;
        glm::vec3 offset;
        glm::vec3 normal;

        operator bool() const
        {
            return collides;
        }
    };

    AABBSlopeIntersection AABBSlopeCollision(glm::vec3 posA, glm::vec3 sizeA, glm::vec3 sizeB, float precision = 0.005f);
    AABBSlopeIntersection AABBSlopeCollision(Math::Transform transformAABB, Math::Transform transformSlope, float precision = 0.005f);

    // Points
    bool IsPointInAABB(glm::vec3 point, Math::Transform transformAABB);
    bool IsPointInSphere(glm::vec3 point, glm::vec3 center, float scale);
    struct SpherePointCollision
    {
        bool collides;
        glm::vec3 dist;
        float distance;

        operator bool() const
        {
            return collides;
        }
    };
    SpherePointCollision PointInSphere(glm::vec3 point ,glm::vec3 center, float scale);
};