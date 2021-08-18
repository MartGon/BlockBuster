#include <Map.h>

#include <debug/Debug.h>

// #### Public Interface #### \\

Game::Block* Game::Map::Map::GetBlock(glm::ivec3 pos)
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

void Game::Map::Map::AddBlock(glm::ivec3 pos, Game::Block block)
{
    auto index = ToChunkIndex(pos);
    if(chunks_.find(index) == chunks_.end())
    {
        chunks_[index] = Chunk{};
    }
    
    auto& chunk = chunks_[index];
    auto blockPos = ToBlockChunkPos(pos);
    chunk.SetBlock(blockPos, block);
}

void Game::Map::Map::RemoveBlock(glm::ivec3 pos)
{
    if(auto block = GetBlock(pos))
        block->type = Game::BlockType::NONE;
}

glm::ivec3 Game::Map::Map::ToChunkIndex(glm::ivec3 blockPos)
{
    auto cPos = blockPos + Chunk::HALF_DIMENSIONS;
    glm::ivec3 sign = glm::lessThan(cPos, glm::ivec3{0});
    glm::ivec3 chunkIndex = (cPos + sign) / Chunk::DIMENSIONS + sign * glm::ivec3{-1};
    return chunkIndex;
}

glm::ivec3 Game::Map::Map::ToBlockChunkPos(glm::ivec3 blockPos)
{   
    auto cPos = blockPos + Chunk::HALF_DIMENSIONS;
    auto mod = cPos % (Chunk::DIMENSIONS);
    glm::ivec3 modSign = glm::lessThan(mod, glm::ivec3{0});

    auto blockChunkPos = mod + Chunk::DIMENSIONS * modSign;
    return blockChunkPos;
}

std::vector<glm::ivec3> Game::Map::Map::GetChunkIndices() const
{
    std::vector<glm::ivec3> indices;
    indices.reserve(chunks_.size());
    for(const auto& pair : chunks_) 
        indices.push_back(pair.first);

    return indices;
}

Game::Map::Map::Chunk& Game::Map::Map::GetChunk(glm::ivec3 pos) 
{
    return chunks_.at(pos);
}

void Game::Map::Map::Clear()
{
    chunks_.clear();
}

unsigned int Game::Map::Map::GetBlockCount() const
{
    unsigned int blockCount = 0;
    for(auto chunk : chunks_)
        blockCount += chunk.second.GetBlockCount();

    return blockCount;
}

Game::Map::Map::ChunkIterator Game::Map::Map::CreateChunkIterator()
{
    return ChunkIterator{this, GetChunkIndices()};
}

// #### Iterator #### \\

Game::Map::Map::Iterator Game::Map::Map::CreateIterator()
{
    return Iterator{this, GetChunkIndices()};
}

std::pair<glm::ivec3, Game::Block*> Game::Map::Map::Iterator::GetNextBlock()
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

bool Game::Map::Map::Iterator::IsOver() const
{
    return end_;
}

// #### Chunk #### \\

Game::Map::Map::Chunk::Chunk()
{
    for(auto x = 0; x < CHUNK_WIDTH; x++)
    {
        for(auto y = 0; y < CHUNK_HEIGHT; y++)
        {
            for(auto z = 0; z < CHUNK_DEPTH; z++)
            {
                auto index = ToIndex(glm::ivec3{x, y, z});
                blocks_[index] = Game::Block{ 
                    Game::NONE, ROT_0, ROT_0, Game::Display{Game::DisplayType::COLOR, 2}
                };
            }
        }
    }
}

Game::Map::Map::Chunk::BlockIterator Game::Map::Map::Chunk::CreateBlockIterator()
{
    return BlockIterator{this};
}

Game::Block* Game::Map::Map::Chunk::GetBlock(glm::ivec3 pos)
{
    auto index = ToIndex(pos);
    return &blocks_[index];
}

void Game::Map::Map::Chunk::SetBlock(glm::ivec3 pos, Game::Block block)
{
    auto index = ToIndex(pos);
    blocks_[index] = block;
}

unsigned int Game::Map::Map::Chunk::GetBlockCount() const
{
    unsigned int blockCount = 0;
    for(int i = 0; i < Chunk::CHUNK_BLOCKS; i++)
        if(blocks_[i].type != Game::BlockType::NONE)
            blockCount++;

    return blockCount;
}

int Game::Map::Map::Chunk::ToIndex(glm::ivec3 pos)
{
    bool valid = pos.x < DIMENSIONS.x && pos.y < DIMENSIONS.y && pos.z < DIMENSIONS.z &&
                pos.x >= 0 && pos.y >= 0 && pos.z >= 0;
    assertm(valid, "Invalid chunk pos");
    return pos.x + pos.y * CHUNK_WIDTH + pos.z * CHUNK_WIDTH * CHUNK_HEIGHT;
}

// #### BlockIterator #### \\

std::pair<glm::ivec3, Game::Block*> Game::Map::Map::Chunk::BlockIterator::GetNextBlock()
{
    Game::Block* block = nullptr;
    auto pos = index_;
    while(!end_ && (block == nullptr || block->type == Game::BlockType::NONE))
    {
        block = chunk_->GetBlock(index_);
        pos = index_;

        // Update index
        if(index_.z < Chunk::CHUNK_DEPTH - 1)
            index_.z++;
        else if(index_.y < Chunk::CHUNK_HEIGHT - 1)
        {
            index_.z = 0;
            index_.y++;
        }
        else if(index_.x < Chunk::CHUNK_WIDTH - 1)
        {
            index_.z = 0;
            index_.y = 0;
            index_.x++;
        }
        else
            end_ = true;
    }
    auto blockPos = pos - Chunk::HALF_DIMENSIONS;
    return {blockPos, block};
}

bool Game::Map::Map::Chunk::BlockIterator::IsOver() const
{
    return end_;
}

// #### ChunkIterator #### \\

std::pair<glm::ivec3, Game::Map::Map::Chunk*> Game::Map::Map::ChunkIterator::GetNextChunk()
{
    Game::Map::Map::Chunk* chunk = nullptr;
    glm::ivec3 pos;
    if(metaIndex_ < chunkIndices_.size())
    {
        pos = chunkIndices_[metaIndex_++];
        chunk = &map_->GetChunk(pos);
    }
    else
        end_ = true;

    return {pos, chunk};
}

bool Game::Map::Map::ChunkIterator::IsOver() const
{
    return end_;
}

// #### Private Interface #### \\


// #### Map Namespace #### \\

glm::vec3 Game::Map::ToGlobalChunkPos(glm::ivec3 chunkIndex, float blockScale)
{
    return glm::vec3{chunkIndex} * glm::vec3{Game::Map::Map::Chunk::DIMENSIONS} * blockScale;
}

glm::ivec3 Game::Map::ToGlobalBlockPos(glm::ivec3 chunkIndex, glm::ivec3 blockIndex)
{
    return glm::vec3{chunkIndex} * glm::vec3{Game::Map::Map::Chunk::DIMENSIONS} + glm::vec3{blockIndex};
}

glm::ivec3 Game::Map::ToChunkPos(glm::vec3 globalPos, float blockScale)
{
    glm::vec3 pos = ToBlockPos(globalPos, blockScale) + Game::Map::Map::Chunk::HALF_DIMENSIONS;
    return glm::floor(pos / glm::vec3{Game::Map::Map::Chunk::DIMENSIONS});
}

glm::ivec3 Game::Map::ToBlockPos(glm::vec3 globalPos, float blockScale)
{
    return glm::round(globalPos / blockScale);
}
