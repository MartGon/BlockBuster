#include <Game.h>

#include <algorithm>

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

Game::RayPlayerCollision Game::RayCollidesWithPlayer(Collisions::Ray ray, glm::vec3 playerPos, float playerYaw, glm::vec3 lastMoveDir)
{
    using HitBoxType = Entity::Player::HitBoxType;
    
    Game::RayPlayerCollision ret;
    for(uint8_t i = HitBoxType::HEAD; i < HitBoxType::MAX; i++)
    {
        auto rot = i == HitBoxType::WHEELS ? Entity::Player::GetWheelsRotation(lastMoveDir, playerYaw + 90.0f) : playerYaw;
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