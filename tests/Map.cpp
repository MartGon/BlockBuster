#include <doctest/doctest.h>

#include <entity/Map.h>
#include <util/Random.h>

TEST_CASE("Block Init")
{
    using namespace Game;
    using Chunk = Map::Map::Chunk;
    Map::Map map;

    // INit map
    auto chunks = 512;
    for(auto i = 0; i < Chunk::CHUNK_WIDTH * chunks; i += Chunk::CHUNK_WIDTH)
    {
        uint32_t min = 20;
        uint32_t max = 240;
        auto chunkBlocks = Util::Random::Uniform(min, max);
        chunkBlocks = 300;
        auto placedBlocks = 0;
        for(auto x = -7; x < 7; x++)
        {
            if(placedBlocks >= chunkBlocks)
                break;
            for(auto y = -7; y < 7; y++)
            {
                if(placedBlocks >= chunkBlocks)
                    break;
                for(auto z = -7; z < 7; z++)
                {
                    if(placedBlocks >= chunkBlocks)
                        break;

                    auto intType = Util::Random::Uniform(0u, 2u) + 1u;
                    auto type = static_cast<BlockType>(intType);
                    map.AddBlock(glm::ivec3{x, y, z} + glm::ivec3{i}, Block{type, BlockRot{RotType::ROT_0, RotType::ROT_0}, Display{DisplayType::TEXTURE, 0}});
                    placedBlocks++;
                }
            }            
        }
    }

    auto chunkIndices = map.GetChunkIndices();
    for(auto chunkIndex : chunkIndices)
    {   
        auto& chunk = map.GetChunk(chunkIndex);
        auto blockCount = chunk.GetBlockCount();

        int countedBlocks = 0;
        auto chunkIt = chunk.CreateBlockIterator();
        for(auto b = chunkIt.GetNextBlock(); !chunkIt.IsOver(); b = chunkIt.GetNextBlock())
        {
            countedBlocks++;
        }

        CHECK(countedBlocks == blockCount);
        CHECK(blockCount == 300);
    }

    auto buffer = map.ToBuffer();
    auto reader = buffer.GetReader();
    Map::Map loaded = Map::Map::FromBuffer(reader);

    chunkIndices = loaded.GetChunkIndices();
    for(auto chunkIndex : chunkIndices)
    {   
        auto& chunk = loaded.GetChunk(chunkIndex);
        auto blockCount = chunk.GetBlockCount();

        int countedBlocks = 0;
        auto chunkIt = chunk.CreateBlockIterator();
        for(auto b = chunkIt.GetNextBlock(); !chunkIt.IsOver(); b = chunkIt.GetNextBlock())
        {
            countedBlocks++;
        }

        CHECK(countedBlocks == blockCount);
        CHECK(blockCount == 300);
    }
}