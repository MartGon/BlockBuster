#include <PlayerController.h>

#include <collisions/Collisions.h>

#include <debug/Debug.h>

#include <algorithm>
#include <iostream>

void Entity::PlayerController::Update(Entity::PlayerInput input, Game::Map::Map* map, Util::Time::Seconds deltaTime)
{
    // Move
    glm::vec3 moveDir{0.0f};
    if(input[Entity::MOVE_LEFT])
        moveDir.x -= 1;
    if(input[Entity::MOVE_RIGHT])
        moveDir.x += 1;
    if(input[Entity::MOVE_UP])
        moveDir.z -= 1;
    if(input[Entity::MOVE_DOWN])
        moveDir.z += 1;

    // Rotate moveDir
    auto rotMat = transform.GetRotationMat();
    moveDir = glm::vec3{rotMat * glm::vec4{moveDir, 1.0f}};
    bool isMoving = glm::length(moveDir) > 0.0f;

    if(isMoving)
    {
        moveDir = isMoving ? glm::normalize(moveDir) : moveDir;
        
        // Horizontal movement
        glm::vec3 velocity = moveDir * speed * (float) deltaTime.count();
        transform.position += velocity;

        auto offset = HandleCollisions(map, 2.0f, false);
        transform.position += offset;
        Debug::PrintVector(offset, "Horizontal Collision Offset");
    }

    // Gravity effect
    auto velocity = glm::vec3{0.0f, gravitySpeed, 0.0f};
    transform.position += velocity;

    auto offset = HandleCollisions(map, 2.0f, true);
    transform.position.y += offset.y;
    Debug::PrintVector(offset, "Gravity Collision Offset");
    Debug::PrintVector(transform.position, "Position");
}

union IntersectionU
{
    Collisions::AABBSlopeIntersection aabbSlope;
    Collisions::AABBIntersection aabb;
};

struct Intersection
{
    Game::Block block;
    IntersectionU intersection;
    Math::Transform blockTransform;
};

glm::vec3 Entity::PlayerController::HandleCollisions(Game::Map::Map* map, float blockScale, bool gravity)
{
    std::vector<std::pair<Math::Transform, Game::Block>> blocks;
    auto bIt = map->CreateIterator();
    for(auto b = bIt.GetNextBlock(); !bIt.IsOver(); b = bIt.GetNextBlock())
        blocks.push_back({Game::GetBlockTransform(*b.second, b.first, blockScale), *b.second});

    return HandleCollisions(blocks, gravity);
}

glm::vec3 Entity::PlayerController::HandleCollisions(const std::vector<std::pair<Math::Transform, Game::Block>> &blocks, bool gravity)
{
    glm::vec3 offset{0.0f};

    auto mcb = Entity::Player::moveCollisionBox;
    auto mct = this->transform;
    mct.position += mcb.position;
    mct.scale = mcb.scale;

    bool intersects;
    unsigned int iterations = 0;
    const unsigned int MAX_ITERATIONS = 10;
    std::vector<glm::vec<3, int>> alreadyOffset;
    do
    {        
        intersects = false;
        iterations++;

        std::vector<Intersection> intersections;
        for(const auto& [blockTransform, block] : blocks)
        {            
            if(block.type == Game::SLOPE)
            {
                // Collision player and slope
                auto slopeIntersect = Collisions::AABBSlopeCollision(mct, blockTransform);
                if(slopeIntersect.collides)
                {
                    if(slopeIntersect.intersects)
                    {
                        std::cout << "Intersects w Slope at " << glm::to_string(blockTransform.position) << "\n";
                        Intersection intersect;
                        intersect.block = block;
                        intersect.intersection.aabbSlope = slopeIntersect;
                        intersect.blockTransform = blockTransform;
                        // Ignore in gravity check and doesn't push up
                        if(!gravity || (gravity && slopeIntersect.normal.y > 0.0f))
                            intersections.push_back(intersect);

                        intersects = true;
                    }
                }
            }
            else
            {
                auto boxIntersect = Collisions::AABBCollision(mct.position, glm::vec3{mct.scale}, blockTransform.position, blockTransform.scale);
                if(boxIntersect.collides)
                {
                    if(boxIntersect.intersects)
                    {
                        std::cout << "Intersects w Block at " << glm::to_string(blockTransform.position) << "\n";
                        Intersection intersect;
                        intersect.block = block;
                        intersect.intersection.aabb = boxIntersect;
                        intersect.blockTransform = blockTransform;
                        if(!gravity || (gravity && boxIntersect.normal.y > 0.0f))
                            intersections.push_back(intersect);
                        
                        intersects = true;
                    }
                }
            }
        }

        auto playerPos = transform.position;
        std::sort(intersections.begin(), intersections.end(), [playerPos](const auto& a, const auto& b){
            auto distToA = glm::length(a.blockTransform.position - playerPos);
            auto distToB = glm::length(b.blockTransform.position - playerPos);
            return distToA < distToB;
        });

        if(!intersections.empty())
        {
            auto first = intersections.front();
            auto block = first.block;
            Debug::PrintVector("Block Col Handled", first.blockTransform.position);
            Debug::PrintVector("Offset", first.intersection.aabb.offset);
            if(block.type == Game::SLOPE)
            {
                auto slopeIntersect = first.intersection.aabbSlope;
                offset += slopeIntersect.offset;

                auto modPos = gravity ? slopeIntersect.offset * glm::vec3{0.0f, 1.0f, 0.0f} : slopeIntersect.offset;
                mct.position += modPos;
            }
            else
            {
                auto boxIntersect = first.intersection.aabb;
                offset += boxIntersect.offset;

                auto modPos = gravity ? boxIntersect.offset * glm::vec3{0.0f, 1.0f, 0.0f} : boxIntersect.offset;
                mct.position += modPos;
            }

            auto o = first.intersection.aabb.offset;
            alreadyOffset.push_back(first.blockTransform.position);
        }

    }while(intersects && iterations < MAX_ITERATIONS);

    return offset;
}