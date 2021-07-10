#include <Collisions.h>

using namespace Collisions;

// Ray

glm::vec3 Ray::GetDir() const
{
    return glm::normalize(this->dest - this->origin);
}


// RayIntersections

RayIntersection Collisions::RayAABBIntersection(Ray modelRay, glm::vec3 boxSize)
{
    auto rayOrigin = modelRay.origin;
    auto rayDir = modelRay.GetDir();
    glm::vec3 m = 1.0f / rayDir;
    m = glm::clamp(m,-1e30f,1e30f);
    glm::vec3 n = m * rayOrigin;
    glm::vec3 k = glm::abs(m) * boxSize;
    auto t1 = -n - k;
    auto t2 = -n + k;
    float tN = glm::max(glm::max(t1.x, t1.y), t1.z);
    float tF = glm::min(glm::min(t2.x, t2.y), t2.z);

    bool intersection = !(tN > tF);
    auto step1 = glm::step(glm::vec3{t1.y, t1.z, t1.x}, t1);
    auto step2 = glm::step(glm::vec3{t1.z, t1.x, t1.y}, t1);
    auto normal = -glm::sign(rayDir) * step1 * step2;

    return RayIntersection{intersection, glm::vec2{tN, tF}, normal};
}

RayIntersection Collisions::RayAABBIntersection(Ray worldRay, glm::mat4 modelMat)
{
    auto modelRay = ToModelSpace(worldRay, modelMat);
    return RayAABBIntersection(modelRay, glm::vec3{0.5f});
}

bool RaySlopeIntersectionCheckPoint(glm::vec3 point)
{   
    bool intersects = false;
    auto min = glm::sign(point) * glm::step(glm::abs(glm::vec3{point.y, point.z, point.x}), glm::abs(point)) 
        * glm::step(glm::abs(glm::vec3{point.z, point.x, point.y}), glm::abs(point));

    if(min.z == -1 || min.y == -1)
    {
        intersects = true;
    }

    if(min.x == 1 || min.x == -1)
    {        
        intersects = point.y <= (-1 * point.z);
    }

    return intersects;
}

RayIntersection Collisions::RaySlopeIntersection(Ray modelRay, glm::vec3 boxSize)
{
    auto aabbIntersect = Collisions::RayAABBIntersection(modelRay, boxSize);
    RayIntersection intersection{false, aabbIntersect.ts, aabbIntersect.normal};
    if(aabbIntersect.intersects)
    {
        auto rayDir = modelRay.GetDir();
        auto normal = aabbIntersect.normal;
        auto nearPoint = modelRay.origin + rayDir * aabbIntersect.ts.x;
        auto farPoint = modelRay.origin + rayDir * aabbIntersect.ts.y;

        auto nearInt = RaySlopeIntersectionCheckPoint(nearPoint);
        auto farInt = RaySlopeIntersectionCheckPoint(farPoint);
        intersection.intersects = nearInt || farInt;
    }

    return intersection;
}

RayIntersection Collisions::RaySlopeIntersection(Ray worldRay, glm::mat4 modelMat)
{
    auto modelRay = ToModelSpace(worldRay, modelMat);
    return RaySlopeIntersection(modelRay, glm::vec3{0.5f});
}

Ray Collisions::ToModelSpace(Ray ray, glm::mat4 modelMat)
{
    glm::mat4 worldToModel = glm::inverse(modelMat);

    glm::vec3 modelRayOrigin = glm::vec3{worldToModel * glm::vec4(ray.origin, 1.0f)};
    glm::vec3 modelRayDest = worldToModel * glm::vec4{ray.dest, 1.0f};

    return Ray{modelRayOrigin, modelRayDest};
}

// AABB

AABBIntersection Collisions::AABBCollision(glm::vec3 posA, glm::vec3 sizeA, glm::vec3 posB, glm::vec3 sizeB)
{
    // Move to down-left-back corner
    posA = posA - sizeA * 0.5f;
    posB = posB - sizeB * 0.5f;

    auto boundA = posA + sizeA;
    auto boundB = posB + sizeB;
    auto diffA = boundB - posA;
    auto diffB = boundA - posB;

    auto sign = glm::sign(posA - posB);
    auto min = glm::min(diffA, diffB);
    auto minAxis = glm::step(min, glm::vec3{min.z, min.x, min.y}) * glm::step(min, glm::vec3{min.y, min.z, min.x});
    auto normal = minAxis * sign;
    auto offset = sign * min * minAxis;

    auto collision = glm::greaterThanEqual(min, glm::vec3{0.0f}) && glm::lessThan(min, glm::vec3{sizeA + sizeB});
    bool intersects = collision.x && collision.y && collision.z;

    return AABBIntersection{intersects, offset, min, normal};
}

AABBIntersection Collisions::AABBSlopeCollision(glm::vec3 posA, glm::vec3 sizeA, glm::vec3 sizeB, float precision)
{
    auto posB = glm::vec3{0.0f};

    posA = posA - sizeA * 0.5f;
    posB = posB - sizeB * 0.5f;

    auto boundA = posA + sizeA;
    auto boundB = posB + sizeB;
    auto diffA = boundB - posA;
    auto diffB = boundA - posB;

    auto distance = posA - posB;
    auto sign = glm::sign(distance);
    auto min = glm::min(diffA, diffB);

    // Checking for collision with slope side
    bool isAbove = sign.y >= 0.0f;
    bool isInFront = sign.z > 0.0f;
    if(isInFront && isAbove)
    {   
        min.y = diffA.z - (posA.y - posB.y);
        min.z = diffA.y - (posA.z - posB.z);

        // Dealing with floating point precision
        min.y = (float)round(min.y / precision) * precision;
        min.z = (float)round(min.z / precision) * precision;

        // This makes it so it's pushed along the slope normal;
        min.y *= 0.5f;
        min.z *= 0.5f;
    }

    // Collision detection
    auto collision = glm::greaterThanEqual(min, glm::vec3{0.0f}) && glm::lessThan(min, glm::vec3{sizeA + sizeB});
    auto intersects = collision.x && collision.y && collision.z;

    // Offset and normal calculation
    auto minAxis = glm::step(min, glm::vec3{min.z, min.x, min.y}) * glm::step(min, glm::vec3{min.y, min.z, min.x});
    auto normal = sign * minAxis;
    auto offset = sign * min * minAxis;

    return AABBIntersection{intersects, offset, min, normal};
}