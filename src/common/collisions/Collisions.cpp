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

    bool intersection = !(tN > tF) && tF > 0.0f;
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
    bool collides = collision.x && collision.y && collision.z;
    auto intersects = collides && glm::length(offset) > 0.0f;

    return AABBIntersection{collides, intersects, offset, normal};
}

Collisions::AABBSlopeIntersection Collisions::AABBSlopeCollision(glm::vec3 posA, glm::vec3 sizeA, glm::vec3 sizeB, float precision)
{
    auto posB = glm::vec3{0.0f};

    // Move to down, left, back corner
    posA = posA - sizeA * 0.5f;
    posB = posB - sizeB * 0.5f;

    auto distance = posA - posB;
    distance.y = (float)round(distance.y / precision) * precision;
    auto sign = glm::sign(distance);

    // Right up corner
    auto boundA = posA + sizeA;
    auto boundB = posB + sizeB;

    auto diffA = boundB - posA;
    auto diffB = boundA - posB;
    
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

        min.y *= 0.5f;
        min.z *= 0.5f;

        sign.y = 1.0f;
        sign.z = 1.0f;
    }

    // Collision detection
    auto collision = glm::greaterThanEqual(min, glm::vec3{0.0f}) && glm::lessThan(min, glm::vec3{sizeA + sizeB});
    auto collides = collision.x && collision.y && collision.z;

    // Offset and normal calculation
    auto minAxis = glm::step(min, glm::vec3{min.z, min.x, min.y}) * glm::step(min, glm::vec3{min.y, min.z, min.x});
    auto normal = sign * minAxis;
    auto offset = min * normal;

    auto intersects = collides && glm::length(offset) > precision;

    return AABBSlopeIntersection{collides, intersects, offset, normal};
}

// TODO: Weird things happen when scale is not uniform and slope is rotated in Z axis. This should be fixed by using quaternions
Collisions::AABBSlopeIntersection Collisions::AABBSlopeCollision(Math::Transform transformAABB, Math::Transform transformSlope, float precision)
{
    auto posAABB = transformAABB.position;
    auto scaleAABB = transformAABB.scale;

    auto slopeRot = transformSlope.GetRotationMat();
    auto scaleRot = slopeRot;
    auto slopeTranslation = transformSlope.GetTranslationMat();
    auto toSlopeSpace = glm::inverse(slopeTranslation * slopeRot);

    // FIXME/TODO: Hack to fix collisions with wierd axis rotation
    if(transformSlope.rotation.z == 90.0f && (transformSlope.rotation.y == 90.0f || transformSlope.rotation.y == 270.0f))
        scaleRot = glm::inverse(slopeRot);

    posAABB = toSlopeSpace * glm::vec4{posAABB, 1.0f};
    scaleAABB = glm::abs(scaleRot * glm::vec4{scaleAABB, 1.0f});

    auto intersection = AABBSlopeCollision(posAABB, scaleAABB, transformSlope.scale);
    auto slopeSpaceNormal = intersection.normal;

    intersection.normal = slopeRot * glm::vec4{intersection.normal, 1.0f};

    if (intersection.normal.y > 0.0f && slopeSpaceNormal.z > 0.0f && slopeSpaceNormal.y > 0.0f)
    {
        intersection.offset.z = 0.0f;
        intersection.offset.y *= 2.0f;
    }
    
    intersection.offset = slopeRot * glm::vec4{intersection.offset, 1.0f};

    return intersection;
}