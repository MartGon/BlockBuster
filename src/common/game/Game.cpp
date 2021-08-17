#include <Game.h>

#include <algorithm>

std::vector<Game::RayBlockIntersection> Game::CastRay(Game::Map::Map* map, Collisions::Ray ray, float blockScale)
{
    std::vector<Game::RayBlockIntersection> intersections;
    auto chunkIt = map->CreateChunkIterator();
    for(auto c = chunkIt.GetNextChunk(); !chunkIt.IsOver(); c = chunkIt.GetNextChunk())
    {
        glm::vec3 pos = Game::Map::ToGlobalChunkPos(c.first) * blockScale;
        auto chunk = c.second;
        Math::Transform t{pos, glm::vec3{0.0f}, glm::vec3{chunk->DIMENSIONS} * blockScale};
        auto model = t.GetTransformMat();
        auto rayInt = Collisions::RayAABBIntersection(ray, model);

        if(rayInt.intersects)
        {
            auto blockIt = chunk->CreateBlockIterator();
            for(auto b = blockIt.GetNextBlock(); !blockIt.IsOver(); b = blockIt.GetNextBlock())
            {   
                auto block = b.second;
                auto globalPos = Map::ToGlobalBlockPos(c.first, b.first);
                auto realPos = glm::vec3{globalPos} * blockScale;
                auto rot = block->GetRotation();
                Math::Transform bt{realPos, rot, glm::vec3{blockScale}};
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

Game::RayBlockIntersection Game::CastRayFirst(Game::Map::Map* map, Collisions::Ray ray, float blockScale)
{
    Game::RayBlockIntersection intersect = {glm::vec3{0}, nullptr, Collisions::RayIntersection{false}};

    // Sort chunks by distance to origin in global coordinates
    auto chunkIndices = map->GetChunkIndices();
    auto rayOrigin = ray.origin;
    std::sort(chunkIndices.begin(), chunkIndices.end(), [rayOrigin](glm::ivec3 a, glm::ivec3 b)
    {
        auto aDist = glm::length(Game::Map::ToGlobalChunkPos(a) - rayOrigin);
        auto bDist = glm::length(Game::Map::ToGlobalChunkPos(b) - rayOrigin);
        return aDist < bDist;
    });

    for(const auto& chunkIndex : chunkIndices)
    {
        glm::vec3 pos = Game::Map::ToGlobalChunkPos(chunkIndex) * blockScale;
        Math::Transform t{pos, glm::vec3{0.0f}, glm::vec3{Game::Map::Map::Chunk::DIMENSIONS} * blockScale};
        auto model = t.GetTransformMat();
        auto rayInt = Collisions::RayAABBIntersection(ray, model);
        if(rayInt.intersects)
        {
            using BlockData = std::pair<glm::ivec3, Game::Block*>;

            // Get blocks in chunk
            std::vector<BlockData> blocksData;
            auto& chunk = map->GetChunk(chunkIndex);
            auto chunkIt = chunk.CreateBlockIterator();
            for(auto bData = chunkIt.GetNextBlock(); !chunkIt.IsOver(); bData = chunkIt.GetNextBlock())
                blocksData.push_back({Game::Map::ToGlobalBlockPos(chunkIndex, bData.first), bData.second});

            // Sort them by distance in global coordinates to ray origin
            std::sort(blocksData.begin(), blocksData.end(), [rayOrigin](BlockData a, BlockData b)
            {
                auto distA = glm::length(glm::vec3{a.first} - rayOrigin);
                auto distB = glm::length(glm::vec3{b.first} - rayOrigin);
                return distA < distB;
            });

            // Check collision
            for(const auto& bData : blocksData)
            {
                glm::ivec3 globalPos = bData.first;
                Game::Block* block = bData.second;

                glm::vec3 realPos = glm::vec3{globalPos} * blockScale;
                glm::vec3 rot = block->GetRotation();
                Math::Transform bt{realPos, rot, glm::vec3{blockScale}};
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