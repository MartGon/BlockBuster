#include <Map.h>

#include <debug/Debug.h>

Game::Block* Game::Map::GetBlock(glm::ivec3 pos)
{
    Game::Block* block = nullptr;
    auto index = ToChunkIndex(pos);
    if(chunks_.find(index) != chunks_.end())
    {
        auto& chunk = chunks_[index];
        auto blockPos = ToBlockChunkPos(pos);
        block = chunk.GetBlock(blockPos);
    }

    return block;
}

void Game::Map::AddBlock(glm::ivec3 pos, Game::Block block)
{
    auto index = ToChunkIndex(pos);
    if(chunks_.find(index) == chunks_.end())
        chunks_[index] = Chunk{};
    
    auto& chunk = chunks_[index];
    auto blockPos = ToBlockChunkPos(pos);
    chunk.SetBlock(blockPos, block);
}

std::vector<glm::ivec3> Game::Map::GetChunkIndices() const
{
    std::vector<glm::ivec3> indices;
    indices.reserve(chunks_.size());
    for(const auto& pair : chunks_) 
        indices.push_back(pair.first);

    return indices;
}

Game::Map::Chunk& Game::Map::GetChunk(glm::ivec3 pos) 
{
    return chunks_.at(pos);
}

glm::ivec3 Game::Map::ToChunkIndex(glm::ivec3 blockPos)
{
    glm::ivec3 chunkIndex = (blockPos + Chunk::HALF_DIMENSIONS) / Chunk::DIMENSIONS;
    return chunkIndex;
}

glm::ivec3 Game::Map::ToBlockChunkPos(glm::ivec3 blockPos)
{
    auto chunkPos = (blockPos + Chunk::HALF_DIMENSIONS) % dimensions_;
    return chunkPos;
}

Game::Map::Chunk::Chunk()
{
    for(auto x = 0; x < CHUNK_WIDTH; x++)
    {
        for(auto y = 0; y < CHUNK_HEIGHT; y++)
        {
            for(auto z = 0; z < CHUNK_DEPTH; z++)
            {
                auto index = ToIndex(glm::ivec3{x, y, z});
                blocks_[index] = Game::Block{
                    Math::Transform{glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.0f}}, 
                    Game::NONE, Game::Display{Game::DisplayType::COLOR, 2}
                };
            }
        }
    }
}

Game::Block* Game::Map::Chunk::GetBlock(glm::ivec3 pos)
{
    auto index = ToIndex(pos);
    return &blocks_[index];
}

void Game::Map::Chunk::SetBlock(glm::ivec3 pos, Game::Block block)
{
    auto index = ToIndex(pos);
    blocks_[index] = block;
}

int Game::Map::Chunk::ToIndex(glm::ivec3 pos)
{
    bool valid = pos.x < DIMENSIONS.x && pos.y < DIMENSIONS.y && pos.z < DIMENSIONS.z &&
                pos.x >= 0 && pos.y >= 0 && pos.z >= 0;
    assertm(valid, "Invalid chunk pos");
    return pos.x * CHUNK_WIDTH + pos.y * CHUNK_HEIGHT + pos.z;
}