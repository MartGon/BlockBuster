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

Game::Map::Iterator Game::Map::CreateIterator()
{
    return Iterator{this, GetChunkIndices()};
}

std::pair<glm::ivec3, Game::Block*> Game::Map::Iterator::GetNextBlock()
{
    Game::Block* b = nullptr;
    glm::ivec3 pos;
    int index = 0;
    while(!end_ && (b == nullptr || b->type == Game::BlockType::NONE))
    {
        if(chunkMetaIndex < chunkIndices_.size())
        {
            auto chunkIndex = chunkIndices_[chunkMetaIndex];
            auto& chunk = map_->chunks_[chunkIndex];
            
            index = blockOffset_.x * Chunk::CHUNK_WIDTH + blockOffset_.y * Chunk::CHUNK_HEIGHT + blockOffset_.z;
            b = chunk.GetBlock(blockOffset_);
            pos = chunkIndex * Chunk::DIMENSIONS + blockOffset_ - Chunk::HALF_DIMENSIONS;
        }
        else
            end_ = true;

        // Update index
        if(blockOffset_.z < Chunk::CHUNK_DEPTH - 1)
            blockOffset_.z++;
        else if(blockOffset_.y < Chunk::CHUNK_HEIGHT - 1)
        {
            blockOffset_.z = 0;
            blockOffset_.y++;
        }
        else if(blockOffset_.x < Chunk::CHUNK_WIDTH - 1)
        {
            blockOffset_.z = 0;
            blockOffset_.y = 0;
            blockOffset_.x++;
        }
        else
        {
            chunkMetaIndex++;
            blockOffset_ = glm::ivec3{0};
        }
    }

    return {pos, b};
}

bool Game::Map::Iterator::IsOver() const
{
    return chunkIndices_.empty() || end_;
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
    auto chunkPos = (blockPos + Chunk::HALF_DIMENSIONS) % Chunk::DIMENSIONS;
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
    return pos.x + pos.y * CHUNK_WIDTH + pos.z * CHUNK_WIDTH * CHUNK_HEIGHT;
}