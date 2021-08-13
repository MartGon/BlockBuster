#include <Game.h>

std::vector<Game::RayBlockIntersection> Game::CastRay(Game::Map* map, Collisions::Ray ray, float blockScale)
{
    std::vector<Game::RayBlockIntersection> intersections;
    auto chunkIt = map->CreateChunkIterator();
    for(auto c = chunkIt.GetNextChunk(); !chunkIt.IsOver(); c = chunkIt.GetNextChunk())
    {
        glm::vec3 pos = glm::vec3{c.first} * blockScale;
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
                auto blockPos = b.first + c.first * Game::Map::Chunk::DIMENSIONS;
                glm::vec3 globalPos = glm::vec3{blockPos} * blockScale;
                auto rot = glm::vec3{0.0f, block->rot.y * 90.0f, block->rot.z * 90.0f};
                Math::Transform bt{globalPos, rot, glm::vec3{blockScale}};
                auto model = bt.GetTransformMat();

                Collisions::RayIntersection blockInt;
                if(block->type == Game::SLOPE)
                    blockInt = Collisions::RaySlopeIntersection(ray, model);
                else
                    blockInt = Collisions::RayAABBIntersection(ray, model);

                if(blockInt.intersects)
                {
                    Game::RayBlockIntersection rbi{blockPos, block, blockInt};
                    intersections.push_back(rbi);
                }
            }
        }
    }

    return intersections;
}