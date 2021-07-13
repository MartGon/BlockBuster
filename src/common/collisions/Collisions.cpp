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
    auto intersection = RayAABBIntersection(modelRay, glm::vec3{0.5f});
    intersection.normal = ToWorldSpace(intersection.normal, modelMat);
    return intersection;
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
    auto intersection = RaySlopeIntersection(modelRay, glm::vec3{0.5f});
    intersection.normal = ToWorldSpace(intersection.normal, modelMat);
    return intersection;
}

Ray Collisions::ToModelSpace(Ray ray, glm::mat4 modelMat)
{
    glm::mat4 worldToModel = glm::inverse(modelMat);

    glm::vec3 modelRayOrigin = glm::vec3{worldToModel * glm::vec4(ray.origin, 1.0f)};
    glm::vec3 modelRayDest = worldToModel * glm::vec4{ray.dest, 1.0f};

    return Ray{modelRayOrigin, modelRayDest};
}

glm::vec3 Collisions::ToWorldSpace(glm::vec3 normal, glm::mat4 modelMat)
{
    glm::mat4 rotMat = glm::mat4{glm::mat3{modelMat}};
    normal = glm::normalize(glm::vec3{rotMat * glm::vec4{normal, 1.0f}});
    return normal;
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

    return AABBIntersection{intersects, offset, normal};
}

Collisions::AABBSlopeIntersection Collisions::AABBSlopeCollision(glm::vec3 posA, glm::vec3 sizeA, glm::vec3 sizeB, float precision)
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

    return AABBSlopeIntersection{intersects, offset, normal, normal};
}

Collisions::AABBSlopeIntersection Collisions::AABBSlopeCollision(Math::Transform transformAABB, Math::Transform transformSlope)
{
    auto posAABB = transformAABB.position;
    auto slopeRot = transformSlope.GetRotationMat();
    auto toSlopeSpace = glm::inverse(transformSlope.GetTranslationMat() * slopeRot);
    posAABB = toSlopeSpace * glm::vec4{posAABB, 1.0f};

    auto intersection = AABBSlopeCollision(posAABB, glm::vec3{1.f}, glm::vec3{transformSlope.scale});

    intersection.offset = slopeRot * glm::vec4{intersection.offset, 1.0f};
    intersection.normal = slopeRot * glm::vec4{intersection.normal, 1.0f};

    return intersection;
}