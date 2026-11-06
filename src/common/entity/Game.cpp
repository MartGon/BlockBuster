#include <Game.h>

#include <algorithm>
#include <limits>

std::vector<Game::RayBlockIntersection> Game::CastRay(Game::Map::Map* map, Collisions::Ray ray, float blockScale)
{
    std::vector<Game::RayBlockIntersection> intersections;
    auto chunkIt = map->CreateChunkIterator();
    for(auto c = chunkIt.GetNextChunk(); !chunkIt.IsOver(); c = chunkIt.GetNextChunk())
    {
        glm::vec3 pos = Game::Map::ToRealChunkPos(c.first, blockScale);
        Math::Transform t{pos, glm::vec3{0.0f}, glm::vec3{Game::Map::Map::Chunk::DIMENSIONS} * blockScale};
        auto model = t.GetTransformMat();
        auto rayInt = Collisions::RayAABBIntersection(ray, model);

        if(rayInt.intersects)
        {
            auto blockIt = c.second->CreateBlockIterator();
            for(auto b = blockIt.GetNextBlock(); !blockIt.IsOver(); b = blockIt.GetNextBlock())
            {   
                auto block = b.second;
                auto globalPos = Map::ToGlobalPos(c.first, b.first);
                Math::Transform bt = Game::GetBlockTransform(globalPos, block->rot, blockScale);
                auto model = bt.GetTransformMat();

                Collisions::RayIntersection blockInt;
                if(block->type == Game::SLOPE)
                    blockInt = Collisions::RaySlopeIntersection(ray, model);
                else
                    blockInt = Collisions::RayAABBIntersection(ray, model);

                if(blockInt.intersects)
                {
                    Game::RayBlockIntersection rbi{globalPos, block, blockInt};
                    intersections.push_back(rbi);
                }
            }
        }
    }

    return intersections;
}

// TODO: Change map to const&
Game::RayBlockIntersection Game::CastRayFirst(Game::Map::Map* map, Collisions::Ray ray, float blockScale)
{
    Game::RayBlockIntersection intersect = {glm::vec3{0}, nullptr, Collisions::RayIntersection{false}};

    // Sort chunks by distance to origin in global coordinates
    auto chunkIndices = map->GetChunkIndices();
    auto rayOrigin = ray.origin;
    std::sort(chunkIndices.begin(), chunkIndices.end(), [rayOrigin, blockScale](const auto& a, const auto& b)
    {
        auto aDist = glm::length(glm::vec3{a - Game::Map::ToChunkIndex(rayOrigin, blockScale)});
        auto bDist = glm::length(glm::vec3{b - Game::Map::ToChunkIndex(rayOrigin, blockScale)});
        return aDist < bDist;
    });

    for(const auto& chunkIndex : chunkIndices)
    {
        glm::vec3 pos = Game::Map::ToRealChunkPos(chunkIndex, blockScale);
        glm::vec3 size = glm::vec3{Game::Map::Map::Chunk::DIMENSIONS} * blockScale;
        Math::Transform t{pos, glm::vec3{0.0f}, size};
        auto model = t.GetTransformMat();
        auto rayInt = Collisions::RayAABBIntersection(ray, model);
        if(rayInt.intersects)
        {
            using BlockData = std::pair<glm::ivec3, Game::Block const*>;

            // Get blocks in chunk
            std::vector<BlockData> blocksData;
            auto chunkIt = map->GetChunk(chunkIndex).CreateBlockIterator();
            for(auto bData = chunkIt.GetNextBlock(); !chunkIt.IsOver(); bData = chunkIt.GetNextBlock())
            {
                auto blockPos = Game::Map::ToGlobalPos(chunkIndex, bData.first);
                blocksData.push_back({blockPos, bData.second});
            }

            // Sort them by distance in global coordinates to ray origin
            // WARNING!!!: take references instead of copies or their content will be destroyed
            std::sort(blocksData.begin(), blocksData.end(), [rayOrigin, blockScale](const auto& a, const auto& b)
            {
                auto distA = glm::length(Game::Map::ToRealPos(a.first, blockScale) - rayOrigin);
                auto distB = glm::length(Game::Map::ToRealPos(b.first, blockScale) - rayOrigin);
                return distA < distB;
            });

            // Check collision
            for(const auto& bData : blocksData)
            {
                glm::ivec3 globalPos = bData.first;
                Game::Block const* block = bData.second;

                Math::Transform bt = GetBlockTransform(globalPos, block->rot, blockScale);
                auto model = bt.GetTransformMat();

                Collisions::RayIntersection blockInt;
                if(block->type == Game::SLOPE)
                    blockInt = Collisions::RaySlopeIntersection(ray, model);
                else
                    blockInt = Collisions::RayAABBIntersection(ray, model);

                if(blockInt.intersects)
                {
                    Game::RayBlockIntersection rbi{globalPos, block, blockInt};
                    return rbi;
                }
            }
        }
    }

    return intersect;
}

Game::RayBlockIntersection Game::CastRayFirstDDA(Game::Map::Map* map, Collisions::Ray ray, float blockScale)
{
    using Chunk = Game::Map::Map::Chunk;
    const Game::RayBlockIntersection noHit = {glm::ivec3{0}, nullptr, Collisions::RayIntersection{false}};
    constexpr float INF = std::numeric_limits<float>::infinity();

    auto chunkIndices = map->GetChunkIndices();
    if(chunkIndices.empty() || ray.origin == ray.dest)
        return noHit;

    // Map bounds, in blocks. In grid space, block (x, y, z) spans [x, x + 1]
    glm::ivec3 minChunk = chunkIndices.front();
    glm::ivec3 maxChunk = minChunk;
    for(auto chunkIndex : chunkIndices)
    {
        minChunk = glm::min(minChunk, chunkIndex);
        maxChunk = glm::max(maxChunk, chunkIndex);
    }
    glm::ivec3 minBlock = minChunk * Chunk::DIMENSIONS - Chunk::HALF_DIMENSIONS;
    glm::ivec3 maxBlock = maxChunk * Chunk::DIMENSIONS + Chunk::HALF_DIMENSIONS - 1;

    auto dir = ray.GetDir();
    auto origin = ray.origin / blockScale;

    // Clip the ray to the map bounds. Distances along the ray are in grid units
    float tEnter = 0.0f;
    float tExit = INF;
    for(auto i = 0; i < 3; i++)
    {
        if(dir[i] == 0.0f)
        {
            if(origin[i] < minBlock[i] || origin[i] > maxBlock[i] + 1)
                return noHit;
            continue;
        }

        auto t1 = (minBlock[i] - origin[i]) / dir[i];
        auto t2 = (maxBlock[i] + 1 - origin[i]) / dir[i];
        tEnter = std::max(tEnter, std::min(t1, t2));
        tExit = std::min(tExit, std::max(t1, t2));
    }
    if(tEnter > tExit)
        return noHit;

    auto start = origin + dir * tEnter;
    glm::ivec3 cell = glm::clamp(glm::ivec3{glm::floor(start)}, minBlock, maxBlock);

    // Distance along the ray to the next cell boundary on each axis, and between boundaries
    glm::ivec3 step;
    glm::vec3 tMax;
    glm::vec3 tDelta;
    for(auto i = 0; i < 3; i++)
    {
        if(dir[i] > 0.0f)
        {
            step[i] = 1;
            tDelta[i] = 1.0f / dir[i];
            tMax[i] = tEnter + (cell[i] + 1 - start[i]) / dir[i];
        }
        else if(dir[i] < 0.0f)
        {
            step[i] = -1;
            tDelta[i] = -1.0f / dir[i];
            tMax[i] = tEnter + (start[i] - cell[i]) / -dir[i];
        }
        else
        {
            step[i] = 0;
            tDelta[i] = INF;
            tMax[i] = INF;
        }
    }

    while(true)
    {
        auto block = map->GetBlock(cell);
        if(block && block->type != Game::BlockType::NONE)
        {
            // Exact test. The ray may cross the cell and still miss a slope
            auto model = GetBlockTransform(cell, block->rot, blockScale).GetTransformMat();
            Collisions::RayIntersection blockInt;
            if(block->type == Game::SLOPE)
                blockInt = Collisions::RaySlopeIntersection(ray, model);
            else
                blockInt = Collisions::RayAABBIntersection(ray, model);

            if(blockInt.intersects)
                return Game::RayBlockIntersection{cell, block, blockInt};
        }

        // Step into the next cell, through the closest boundary
        int axis = tMax.x < tMax.y ? (tMax.x < tMax.z ? 0 : 2) : (tMax.y < tMax.z ? 1 : 2);
        if(tMax[axis] > tExit)
            break;

        cell[axis] += step[axis];
        tMax[axis] += tDelta[axis];
    }

    return noHit;
}

Game::RayPlayerCollision Game::RayCollidesWithPlayer(Collisions::Ray ray, glm::vec3 playerPos, float playerYaw, glm::vec3 lastMoveDir)
{
    using HitBoxType = Entity::Player::HitBoxType;
    
    Game::RayPlayerCollision ret;
    for(uint8_t i = HitBoxType::HEAD; i < HitBoxType::MAX; i++)
    {
        auto rot = i == HitBoxType::WHEELS ? Entity::Player::GetWheelsRotation(lastMoveDir, playerYaw ) : playerYaw;
        auto hbt = static_cast<HitBoxType>(i);
        auto rpc = RayCollidesWithPlayerHitbox(ray, playerPos, rot, hbt);
        if(!ret.collides)
            ret = rpc;
        else if(rpc.collides)
        {
            auto newLen = rpc.intersection.GetRayLength(ray);
            auto oldLen = ret.intersection.GetRayLength(ray);
            if(newLen < oldLen)
                ret = rpc;
        }
    }

    return ret;
}

Game::RayPlayerCollision Game::RayCollidesWithPlayerHitbox(Collisions::Ray ray, glm::vec3 playerPos, float playerYaw, Entity::Player::HitBoxType type)
{
    auto playerHitbox = Entity::Player::GetHitBox();
    auto t = playerHitbox[type];
    t.position += playerPos;
    t.rotation.y = playerYaw;

    auto collision = Collisions::RayAABBIntersection(ray, t.GetTransformMat());
    Game::RayPlayerCollision rpc{collision.intersects, type, collision};
    return rpc;
}

Collisions::AABBIntersection Game::AABBCollidesWithPlayer(Math::Transform aabb, glm::vec3 playerPos, float playerYaw, glm::vec3 lastMoveDir)
{
    using HitBoxType = Entity::Player::HitBoxType;

    Collisions::AABBIntersection intersect;
    for(uint8_t i = HitBoxType::HEAD; i < HitBoxType::MAX; i++)
    {
        auto rot = i == HitBoxType::WHEELS ? Entity::Player::GetWheelsRotation(lastMoveDir, playerYaw ) : playerYaw;
        auto hbt = static_cast<HitBoxType>(i);
        auto playerHitbox = Entity::Player::GetHitBox();
        auto t = playerHitbox[i];
        t.position += playerPos;
        t.rotation.y = playerYaw;
        
        intersect = Collisions::AABBCollision(aabb.position, aabb.scale, t.position, t.scale);
        if(intersect.collides)
            return intersect;
    }

    return intersect;
}

Collisions::Intersection Game::AABBCollidesBlock(Game::Map::Map* map, Math::Transform aabb)
{
    Collisions::Intersection ret{false};

    auto blockScale = map->GetBlockScale();
    auto blockPos = Game::Map::ToGlobalPos(aabb.position, blockScale);
    auto as = 1;
    std::vector<glm::vec3> blocks;
    for(auto x = -as; x <= as; x++)
    {
        for(auto y = -as; y <= as; y++)
        {
            for(auto z = -as; z <= as; z++)
            {
                glm::ivec3 offset{x, y, z};
                auto block = blockPos + offset;
                auto rPos = Game::Map::ToRealPos(block, blockScale);
                blocks.push_back(rPos);
            }
        }
    }

    auto pos = aabb.position;
    std::sort(blocks.begin(), blocks.end(), [pos](auto a, auto b){
        auto ad = glm::length(a - pos);
        auto bd = glm::length(b - pos);
        return ad < bd;
    });

    for(auto rPos : blocks)
    {
        auto block = Game::Map::ToGlobalPos(rPos, blockScale);
        if(!map->IsNullBlock(block))
        {
            auto b = map->GetBlock(block);
            auto transform = Game::GetBlockTransform(*b, block, map->GetBlockScale());
            if(b->type == Game::BlockType::BLOCK)
            {
                auto collision = Collisions::AABBCollision(aabb.position, aabb.scale, rPos, glm::vec3{blockScale});
                ret = Collisions::Intersection{collision.collides, collision.normal, collision.offset};
            }
            else if(b->type == Game::BlockType::SLOPE)
            {
                auto collision = Collisions::AABBSlopeCollision(aabb, transform);
                ret = Collisions::Intersection{collision.collides, glm::normalize(collision.normal), collision.offset};
            }

            if(ret.collides)
            {
                auto nextBlock = block + glm::ivec3{ret.normal};
                if(block == nextBlock || map->IsNullBlock(nextBlock) || map->GetBlock(nextBlock)->type != Game::BlockType::SLOPE)
                    return ret;
            }
        }
    }

    return ret;
}

bool Game::IsPlayerInTelOrigin(Game::Map::Map* map, glm::vec3 playerPos)
{
    auto telOrigins = map->FindGameObjectByType(Entity::GameObject::TELEPORT_ORIGIN);
    for(auto goPos : telOrigins)
    {
        auto rPos = Game::Map::ToRealPos(goPos, map->GetBlockScale());
        auto go = map->GetGameObject(goPos);
        auto channelId = std::get<int>(go->properties["Channel ID"].value);

        if(Collisions::IsPointInSphere(playerPos, rPos, Entity::GameObject::ACTION_AREA))
        {
            auto dests = map->FindGameObjectByCriteria([channelId](auto pos, auto go){
                return go.type == Entity::GameObject::TELEPORT_DEST && std::get<int>(go.properties["Channel ID"].value) == channelId;
            });

            if(!dests.empty())
                return true;
        }
    }

    return false;
}