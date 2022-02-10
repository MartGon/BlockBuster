#include <PlayerController.h>

#include <collisions/Collisions.h>
#include <Game.h>

#include <debug/Debug.h>

#include <algorithm>
#include <iostream>

using namespace Entity;

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

        /*
        auto offset = HandleCollisions(map, 2.0f, false);
        transform.position += offset;
        Debug::PrintVector(offset, "Horizontal Collision Offset");
        */
    }

    // Gravity effect
    auto offset = HandleGravityCollisions(map, map->GetBlockScale());
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
    auto ecb = GetECB();
    auto sizeInBlocks = ecb.scale / blockScale;
    
    std::vector<std::pair<Math::Transform, Game::Block>> blocks;
    auto allocation = sizeInBlocks.x * sizeInBlocks.y * sizeInBlocks.z;
    blocks.reserve(allocation);

    auto range = glm::ceil(sizeInBlocks / 2.f);
    auto playerBlockPos = Game::Map::ToGlobalPos(ecb.position, blockScale);
    for(int x = -range.x; x <= range.x; x++)
    {
        for(int y = -range.y; y <= range.y; y++)
        {
            for(int z = -range.z; z <= range.z; z++)
            {
                auto blockIndex = playerBlockPos + glm::ivec3{x, y, z};
                if(auto block = map->GetBlock(blockIndex); block && block->type != Game::BlockType::NONE)
                {
                    Math::Transform t = Game::GetBlockTransform(*block, blockIndex, blockScale);
                    blocks.push_back({t, *block});
                }
            }
        }
    }

    return HandleCollisions(blocks, gravity);
}

Math::Transform PlayerController::GetECB()
{
    auto mcb = Entity::Player::moveCollisionBox;
    auto ecb = this->transform;
    ecb.position += mcb.position;
    ecb.scale = mcb.scale;

    return ecb;
}

// TODO: Clean up gravity parameter.
glm::vec3 Entity::PlayerController::HandleCollisions(const std::vector<std::pair<Math::Transform, Game::Block>> &blocks, bool gravity)
{
    glm::vec3 offset{0.0f};

    auto mct = GetECB();

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
                        // Ignore if gravity check and doesn't push up
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

glm::vec3 PlayerController::HandleGravityCollisions(Game::Map::Map* map, float blockScale)
{
    glm::vec3 offset = glm::vec3{0.0f, gravitySpeed, 0.0f};
    float fallDistance = glm::abs(gravitySpeed); // * deltaTime;

    auto ecb = GetECB();
    auto bottomPoint = ecb.position - glm::vec3{0.0f, ecb.scale.y / 2.0f, 0.0f};
    auto blockIndex = Game::Map::ToGlobalPos(bottomPoint, blockScale);
    auto aboveBlockIndex = blockIndex + glm::ivec3{0, 1, 0};
    auto belowBlockIndex = blockIndex - glm::ivec3{0, 1, 0};

    // Check for block at same height slightly above
    glm::ivec3 blocks[3] = {aboveBlockIndex, blockIndex, belowBlockIndex};
    glm::vec3 dirs[3] = {-offset, offset, offset};
    for(auto i = 0; i < 3; i++)
    {
        auto snap = HandleGravityCollisionBlock(map, bottomPoint, blocks[i], dirs[i]);
        if(snap.has_value())
        {
            offset = snap.value();
            break;
        }
    }

    return offset;
}

std::optional<glm::vec3> PlayerController::HandleGravityCollisionBlock(Game::Map::Map* map, glm::vec3 bottomPoint, glm::ivec3 blockIndex, glm::vec3 rayDir)
{
    std::optional<glm::vec3> offset;

    float blockScale = map->GetBlockScale();
    if(auto block = map->GetBlock(blockIndex); block && block->type != Game::BlockType::NONE)
    {
        auto fallPos = bottomPoint + rayDir;
        Collisions::Ray ray{bottomPoint, fallPos};

        Math::Transform t = Game::GetBlockTransform(*block, blockIndex, blockScale);
        auto tMat = t.GetTransformMat();

        Collisions::RayIntersection intersection;
        if(block->type == Game::BlockType::BLOCK)
        {
            intersection = Collisions::RayAABBIntersection(ray, tMat);
        }
        else if(block->type == Game::BlockType::SLOPE)
        {
            intersection = Collisions::RaySlopeIntersection(ray, tMat);
        }

        if(intersection.intersects)
        {
            const float hover = blockScale * 0.005f;
            auto colPoint = ray.origin + ray.GetDir() * intersection.ts.x + hover;
            if(block->type == Game::BlockType::SLOPE && block->rot.z == Game::RotType::ROT_0)
            {
                auto ssColPoint = glm::inverse(tMat) * glm::vec4{colPoint, 1.0f};
                auto percent = glm::max(0.f, glm::min(1.f - (ssColPoint.z + 0.5f), 1.f));

                colPoint.y = t.position.y + t.scale.y * (percent - 0.5f) + hover;
            }

            auto displacement = colPoint - bottomPoint;
            offset = glm::length(rayDir) > glm::length(displacement) ? displacement : rayDir;
        }
    }

    return offset;
}