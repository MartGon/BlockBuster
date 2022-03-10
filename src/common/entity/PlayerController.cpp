#include <PlayerController.h>

#include <collisions/Collisions.h>
#include <Game.h>

#include <debug/Debug.h>

#include <algorithm>
#include <iostream>

using namespace Entity;

glm::vec3 Entity::PlayerController::UpdatePosition(glm::vec3 pos, float yaw, Entity::PlayerInput input, Game::Map::Map* map, Util::Time::Seconds deltaTime)
{
    auto dT = (float) deltaTime.count();
    transform.position = pos;
    transform.rotation = glm::vec3{0.0f, yaw, 0.0f};

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
    auto rotMat = glm::rotate(glm::mat4{1.0f}, glm::radians(yaw) - glm::half_pi<float>(), glm::vec3{0.0f, 1.0f, 0.0f});
    moveDir = glm::vec3{rotMat * glm::vec4{moveDir, 1.0f}};
    bool isMoving = glm::length(moveDir) > 0.0f;

    if(isMoving)
    {
        moveDir = isMoving ? glm::normalize(moveDir) : moveDir;
        
        // Horizontal movement
        glm::vec3 velocity = moveDir * speed * dT;
        transform.position += velocity;

        auto blocks = GetCollisionBlocks(map, map->GetBlockScale());
        auto offset = HandleCollisions(map, blocks);
        transform.position += offset;
    }

    // Gravity effect
#ifdef ALT_COLLISIONS
    auto offset = HandleGravityCollisionsAlt(map, map->GetBlockScale(), dT);
#else
    auto velocity = glm::vec3{0.0f, gravitySpeed * dT, 0.0f};
    transform.position += velocity;

    auto blocks = GetCollisionBlocks(map, map->GetBlockScale());
    auto offset = HandleGravityCollisions(map, blocks);
#endif

    transform.position.y += offset.y;

    return transform.position;
}

Weapon PlayerController::UpdateWeapon(Weapon weapon, Entity::PlayerInput input, Util::Time::Seconds deltaTime)
{
    const auto weaponType = WeaponMgr::weaponTypes.at(weapon.weaponTypeId);
    switch(weapon.state)
    {
        case Weapon::State::IDLE:
        {
            if(input[Entity::SHOOT] && Entity::HasAmmo(weapon.ammoState, weaponType.ammoData, weaponType.ammoType))
            {
                weapon.state = Weapon::State::SHOOTING;
                weapon.cooldown = weaponType.cooldown;
                weapon.ammoState = Entity::UseAmmo(weapon.ammoState, weaponType.ammoData, weaponType.ammoType);
            }
        }
        break;
        case Weapon::State::SHOOTING:
        {
            weapon.cooldown -= deltaTime;
            if(weapon.cooldown.count() <= 0.0)
                weapon.state = Weapon::State::IDLE;
        }
        break;
        case Weapon::State::RELOADING:
        {

        }
        break;
    }

    return weapon;
}

struct Intersection
{
    Game::Block block;
    union
    {
        Collisions::AABBSlopeIntersection aabbSlope;
        Collisions::AABBIntersection aabb;
    };
    Math::Transform blockTransform;
};

glm::vec3 Entity::PlayerController::HandleCollisions(Game::Map::Map* map, const std::vector<glm::ivec3> &blocks, glm::vec3 normalMask)
{
    // Step 0. Initialize data
    glm::vec3 offset{0.0f};

    auto mct = GetECB();
    
    bool intersects;
    unsigned int iterations = 0;
    const unsigned int MAX_ITERATIONS = 10;
    do
    {        
        intersects = false;
        iterations++;

        // Step 1. Get Collided blocks
        std::vector<Intersection> intersections;
        for(const auto& blockIndex : blocks)
        {    
            auto block = map->GetBlock(blockIndex);
            Math::Transform blockTransform = Game::GetBlockTransform(*block, blockIndex, map->GetBlockScale());

            Intersection intersect;
            intersect.blockTransform = blockTransform;
            intersect.block = *block;
            intersect.aabb.normal = glm::vec3{0.0f};

            if(block->type == Game::SLOPE)
            {
                auto slopeIntersect = Collisions::AABBSlopeCollision(mct, blockTransform);
                if(slopeIntersect.intersects)
                {   
                    intersect.aabbSlope = slopeIntersect;   
                }
            }
            else if(block->type == Game::BLOCK)
            {
                auto boxIntersect = Collisions::AABBCollision(mct.position, glm::vec3{mct.scale}, blockTransform.position, blockTransform.scale);
                if(boxIntersect.intersects)
                {
                    intersect.aabb = boxIntersect;
                }
            }
            
            if(glm::length(intersect.aabb.normal * normalMask) > 0.0f)
            {
                intersections.push_back(intersect);
            }
        }
        intersects = !intersections.empty();

        // Step 2. Sort them by distance
        auto playerPos = transform.position;
        std::sort(intersections.begin(), intersections.end(), [playerPos](const auto& a, const auto& b){
            auto distToA = glm::length(a.blockTransform.position - playerPos);
            auto distToB = glm::length(b.blockTransform.position - playerPos);
            return distToA < distToB;
        });

        // Step 3. Handle collision with closest one
        if(!intersections.empty())
        {
            auto first = intersections.front();
            //Debug::PrintVector("Block Col Handled", first.blockTransform.position);
            //Debug::PrintVector("Offset", first.aabb.offset);

            auto modPos = first.aabb.offset;
            offset += modPos;
            mct.position += modPos;
        }

    }while(intersects && iterations < MAX_ITERATIONS);

    return offset;
}

glm::vec3 Entity::PlayerController::HandleGravityCollisions(Game::Map::Map* map, const std::vector<glm::ivec3> &blocks)
{
    return HandleCollisions(map, blocks, glm::vec3{0.0f, 1.0f, 0.0f});
}

std::vector<glm::ivec3> Entity::PlayerController::GetCollisionBlocks(Game::Map::Map* map, float blockScale)
{
    std::vector<glm::ivec3> blocks;

    auto ecb = GetGCB();
    auto sizeInBlocks = ecb.scale / blockScale;
    
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
                    blocks.push_back({blockIndex});
                }
            }
        }
    }

    return blocks;
}

Math::Transform PlayerController::GetECB()
{
    auto mcb = Entity::Player::GetMoveCollisionBox();
    auto ecb = this->transform;
    ecb.position += mcb.position;
    ecb.scale = mcb.scale;

#ifdef ALT_COLLISIONS
    ecb.position.y += (mcb.scale.z / 4.0f) + 0.01f;
    ecb.scale.y -= (mcb.scale.z / 2.0f);
#endif

    return ecb;
}

Math::Transform PlayerController::GetGCB()
{
    auto mcb = Entity::Player::GetMoveCollisionBox();
    auto ecb = this->transform;
    ecb.position += mcb.position;
    ecb.scale = mcb.scale;

    return ecb;
}

glm::vec3 PlayerController::HandleGravityCollisionsAlt(Game::Map::Map* map, float blockScale, Util::Time::Seconds deltaTime)
{
    float fallDistance = gravitySpeed; // TODO: * deltaTime;
    glm::vec3 offset = glm::vec3{0.0f, fallDistance, 0.0f};

    auto ecb = GetGCB();
    auto bottomPoint = ecb.position - glm::vec3{0.0f, ecb.scale.y / 2.0f, 0.0f};

    auto blockIndex = Game::Map::ToGlobalPos(bottomPoint, blockScale);
    auto aboveBlockIndex = blockIndex + glm::ivec3{0, 1, 0};
    auto belowBlockIndex = blockIndex - glm::ivec3{0, 1, 0};

    glm::ivec3 blocks[3] = {aboveBlockIndex, blockIndex, belowBlockIndex};
    glm::vec3 dirs[3] = {-offset, offset, offset};
    for(auto i = 0; i < 3; i++)
    {
        auto snap = HandleGravityCollisionAltBlock(map, bottomPoint, blocks[i], dirs[i]);
        if(snap.has_value())
        {
            offset = snap.value();
            break;
        }
    }

    return offset;
}

std::optional<glm::vec3> PlayerController::HandleGravityCollisionAltBlock(Game::Map::Map* map, glm::vec3 bottomPoint, glm::ivec3 blockIndex, glm::vec3 rayDir)
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