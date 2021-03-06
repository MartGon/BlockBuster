#include <Map.h>

#include <debug/Debug.h>

#include <util/File.h>

#include <fstream>

using namespace Game::Map;

const int Game::Map::Map::magicNumber = 0xB010F0;

// #### Public Interface #### \\

Game::Block const* Game::Map::Map::GetBlock(glm::ivec3 pos) const
{
    Game::Block const* block = nullptr;
    auto index = ToChunkIndex(pos);
    if(chunks_.find(index) != chunks_.end())
    {
        auto& chunk = chunks_.at(index);
        auto blockPos = ToBlockChunkPos(pos);
        block = chunk.GetBlockByIndex(blockPos);
    }

    return block;
}

void Game::Map::Map::AddBlock(glm::ivec3 pos, Game::Block block)
{
    auto index = ToChunkIndex(pos);
    if(chunks_.find(index) == chunks_.end())
    {
        chunks_[index] = std::move(Chunk{});
    }
    
    auto& chunk = chunks_[index];
    auto blockPos = ToBlockChunkPos(pos);
    chunk.SetBlockByIndex(blockPos, block);
}

void Game::Map::Map::RemoveBlock(glm::ivec3 pos)
{
    if(auto block = GetBlock(pos))
    {
        auto r = *block;
        r.type = Game::BlockType::NONE;
        AddBlock(pos, r);
    }
}

bool Game::Map::Map::IsNullBlock(glm::ivec3 pos) const
{
    auto block = GetBlock(pos);
    return !block || block->type == Game::BlockType::NONE;
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

bool Game::Map::Map::HasChunk(glm::ivec3 index) const
{
    return chunks_.find(index) != chunks_.end();
}

void Game::Map::Map::Clear()
{
    chunks_.clear();
}

unsigned int Game::Map::Map::GetBlockCount() const
{
    unsigned int blockCount = 0;
    for(auto& chunk : chunks_)
        blockCount += chunk.second.GetBlockCount();

    return blockCount;
}

Game::Map::Map::ChunkIterator Game::Map::Map::CreateChunkIterator()
{
    return ChunkIterator{this, GetChunkIndices()};
}

// #### Respawns #### \\

Respawn Respawn::FromGameObject(Entity::GameObject go)
{
    Respawn respawn;
    respawn.pos = go.pos;
    respawn.orientation = std::get<float>(go.properties["Orientation"].value);
    respawn.teamId = static_cast<uint8_t>(std::get<int>(go.properties["TeamId"].value));

    return respawn;
}

Entity::GameObject Respawn::ToGameObject()
{
    auto resGo = Entity::GameObject::Create(Entity::GameObject::Type::RESPAWN);
    resGo.pos = pos;
    resGo.properties["Orientation"].value = this->orientation;
    resGo.properties["TeamId"].value = this->teamId;

    return resGo;
}

void Map::AddRespawn(Respawn respawn)
{
    respawns_[respawn.pos] = respawn;

    if(!GetGameObject(respawn.pos))
        AddGameObject(respawn.ToGameObject());
}

Respawn* Map::GetRespawn(glm::ivec3 pos)
{
    Respawn* respawn = nullptr;
    if(respawns_.find(pos) != respawns_.end())
        respawn = &respawns_[pos];
        
    return respawn;
}

std::vector<glm::ivec3> Map::GetRespawnIndices() const
{
    std::vector<glm::ivec3> vec{};
    for(auto [pos, respawn] : respawns_)
        vec.push_back(pos);

    return vec;
}

void Map::RemoveRespawn(glm::ivec3 pos)
{
    if(auto go = GetGameObject(pos); go->type == Entity::GameObject::Type::RESPAWN)
    {
        gameObjects_.erase(pos);
        respawns_.erase(pos);
    }
}

// #### GameObjects #### \\

void Map::AddGameObject(Entity::GameObject go)
{
    gameObjects_[go.pos] = go;

    if(go.type == Entity::GameObject::Type::RESPAWN && !GetRespawn(go.pos))
        AddRespawn(Respawn::FromGameObject(go));
}

Entity::GameObject* Map::GetGameObject(glm::ivec3 pos)
{
    Entity::GameObject* go = nullptr;
    if(gameObjects_.find(pos) != gameObjects_.end())
        go = &gameObjects_[pos];    
        
    return go;
}

std::vector<glm::ivec3> Map::GetGameObjectIndices() const
{
    std::vector<glm::ivec3> vec{};
    for(auto [pos, go] : gameObjects_)
        vec.push_back(pos);

    return vec;
}

std::vector<glm::ivec3> Map::FindGameObjectByType(Entity::GameObject::Type type) const
{
    std::vector<glm::ivec3> vec;
    for(const auto& [pos, go] : gameObjects_)
    {
        if(go.type == type)
            vec.push_back(pos);
    } 

    return vec;
}

std::vector<glm::ivec3> Map::FindGameObjectByCriteria(std::function<bool(glm::ivec3, Entity::GameObject&)> criteria)
{
    std::vector<glm::ivec3> vec;
    for(auto& [pos, go] : gameObjects_)
    {
        if(criteria(pos, go))
            vec.push_back(pos);
    } 

    return vec;
}

void Map::RemoveGameObject(glm::ivec3 pos)
{
    if(auto go = GetGameObject(pos); go->type == Entity::GameObject::Type::RESPAWN)
        respawns_.erase(pos);

    gameObjects_.erase(pos);
}

// #### Serialization #### \\

Util::Buffer Game::Map::Map::ToBuffer()
{
    Util::Buffer buffer;

    buffer.Write(blockScale);

    // Write map
    auto chunkIndices = GetChunkIndices();

    // Reserving memory to speed up writing
    auto halfMaxBlocks = (Chunk::HALF_DIMENSIONS.x * Chunk::HALF_DIMENSIONS.y * Chunk::HALF_DIMENSIONS.z) / 2;
    auto minMemory = chunkIndices.size() * halfMaxBlocks;
    buffer.Reserve(minMemory);

    buffer.Write(chunkIndices.size());
    for(auto chunkIndex : chunkIndices)
    {   
        auto& chunk = GetChunk(chunkIndex);
        auto i8Index = glm::lowp_i8vec3{chunkIndex};
        buffer.Write(i8Index);

        auto blockCount = chunk.GetBlockCount();
        buffer.Write(blockCount);

        auto countedBlocks = 0;
        auto chunkIt = chunk.CreateBlockIterator();
        for(auto b = chunkIt.GetNextBlock(); !chunkIt.IsOver(); b = chunkIt.GetNextBlock())
        {
            
            // Write pos - block pair
            glm::lowp_i8vec3 pos = b.first;
            buffer.Write(pos);

            auto block = b.second;
            buffer.Write(b.second->type);
            if(block->type == Game::BlockType::SLOPE)
               buffer.Write(block->rot);
            buffer.Write(block->display);

            countedBlocks++;
        }

        assertm(countedBlocks == blockCount, "Blocks counted where not the same!");
    }

    // Write respawns
    buffer.Write(respawns_.size());
    for(auto [id, respawn] : respawns_)
        buffer.Write(respawn);
    
    // Write gos
    auto respawnCount = FindGameObjectByType(Entity::GameObject::RESPAWN).size();
    auto decoyCount = FindGameObjectByType(Entity::GameObject::PLAYER_DECOY).size();
    auto gosToSave = gameObjects_.size() - (respawnCount + decoyCount);
    buffer.Write(gosToSave);
    for(auto& [pos, go] : gameObjects_)
    {
        bool save = go.type != Entity::GameObject::Type::RESPAWN && go.type != Entity::GameObject::Type::PLAYER_DECOY;
        if(save)
            buffer.Append(go.ToBuffer());
    }

    return buffer;
}

Game::Map::Map Game::Map::Map::FromBuffer(Util::Buffer::Reader& reader)
{
    Game::Map::Map map;

    // Read scale
    map.blockScale = reader.Read<float>();

    // Read map
    auto chunkIndicesCount = reader.Read<std::size_t>();
    for(auto i = 0; i < chunkIndicesCount; i++)
    {
        auto chunkIndex = reader.Read<glm::lowp_i8vec3>();
        auto blockCount = reader.Read<unsigned int>();
        for(auto b = 0; b < blockCount; b++)
        {
            auto chunkBlockPos = reader.Read<glm::lowp_i8vec3>();
            auto globalPos = Game::Map::ToGlobalPos(chunkIndex, chunkBlockPos);
            
            Game::Block block;
            block.type = reader.Read<Game::BlockType>();
            if(block.type == Game::BlockType::SLOPE)
                block.rot = reader.Read<Game::BlockRot>();
            block.display = reader.Read<Game::Display>();

            map.AddBlock(globalPos, block);
        }
    }

    // Read respawns
    auto respawnCount = reader.Read<std::size_t>();
    for(auto i = 0; i < respawnCount; i++)
    {
        auto respawn = reader.Read<Respawn>();
        map.AddRespawn(respawn);
    }

    // Read Gameobjects
    auto goCount = reader.Read<std::size_t>();
    for(auto i = 0; i < goCount; i++)
    {
        auto go = Entity::GameObject::FromBuffer(reader);
        map.AddGameObject(go);
    }

    return std::move(map);
}

Result<std::shared_ptr<Map>, Map::LoadMapError> Game::Map::Map::LoadFromFile(std::filesystem::path filePath)
{
    using namespace Util;

    std::fstream file{filePath, file.binary | file.in};
    if(!file.is_open())
    {
       return Err(Map::LoadMapError::FILE_NOT_FOUND);
    }

    auto magic = Util::File::ReadFromFile<int>(file);
    if(magic != magicNumber)
    {
        return Err(Map::LoadMapError::MAGIC_NUMBER_NOT_FOUND);
    }

    // Load map
    auto bufferSize = File::ReadFromFile<uint32_t>(file);
    Util::Buffer buffer = File::ReadFromFile(file, bufferSize);
    auto reader = buffer.GetReader();
    auto map = FromBuffer(reader);

    auto mapPtr = std::make_shared<Map>();
    *mapPtr.get() = std::move(map);

    return Ok(mapPtr);
}

// #### Iterator #### \\

Game::Map::Map::Iterator Game::Map::Map::CreateIterator()
{
    return Iterator{this, GetChunkIndices()};
}

std::pair<glm::ivec3, Game::Block const*> Game::Map::Map::Iterator::GetNextBlock()
{
    Game::Block const* b = nullptr;
    glm::ivec3 pos;
    int index = 0;
    while(!end_ && (b == nullptr || b->type == Game::BlockType::NONE))
    {
        if(chunkMetaIndex < chunkIndices_.size())
        {
            auto chunkIndex = chunkIndices_[chunkMetaIndex];
            auto& chunk = map_->chunks_[chunkIndex];
            
            index = blockOffset_.x * Chunk::CHUNK_WIDTH + blockOffset_.y * Chunk::CHUNK_HEIGHT + blockOffset_.z;
            b = chunk.GetBlockByIndex(blockOffset_);
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

const Game::Block* Game::Map::Map::Chunk::GetBlockByIndex(glm::ivec3 pos) const
{
    auto index = ToIndex(pos);
    return &blocks_[index];
}

void Game::Map::Map::Chunk::SetBlockByIndex(glm::ivec3 pos, Game::Block block)
{
    auto index = ToIndex(pos);
    blocks_[index] = block;

    hasChanged_ = true;
}

Game::Block const* Game::Map::Map::Chunk::GetBlockBySmartIndex(glm::ivec3 smartIndex)
{
    auto index = SmartIndexToIndex(smartIndex);
    return GetBlockByIndex(index);
}

void Game::Map::Map::Chunk::SetBlockBySmartIndex(glm::ivec3 smartIndex, Game::Block block)
{
    auto index = SmartIndexToIndex(smartIndex);
    SetBlockByIndex(index, block);
}

glm::ivec3 Game::Map::Map::Chunk::SmartIndexToIndex(glm::ivec3 smartIndex)
{
    return smartIndex - (glm::ivec3{glm::lessThan(smartIndex, glm::ivec3{0})} * DIMENSIONS);
}

const Game::Block* Game::Map::Map::Chunk::GetBlock(glm::ivec3 pos) const
{
    auto index = PosToIndex(pos);
    return GetBlockByIndex(index);
}

void Game::Map::Map::Chunk::SetBlock(glm::ivec3 pos, Game::Block block)
{
    auto index = PosToIndex(pos);
    SetBlockByIndex(index, block);
}

glm::ivec3 Game::Map::Map::Chunk::PosToIndex(glm::ivec3 pos)
{
    return pos + HALF_DIMENSIONS;
}

Game::Block const* Game::Map::Map::Chunk::GetBlockSmartPos(glm::ivec3 smartPos)
{
    auto index = SmartPosToIndex(smartPos);
    return GetBlockByIndex(index);
}

void Game::Map::Map::Chunk::SetBlockSmartPos(glm::ivec3 smartPos, Game::Block block)
{
    auto index = SmartPosToIndex(smartPos);
    SetBlockByIndex(index, block);
}

glm::ivec3 Game::Map::Map::Chunk::SmartPosToIndex(glm::ivec3 pos)
{
    auto index = pos % 8;
    auto negOffset = (glm::ivec3{glm::lessThan(pos, -HALF_DIMENSIONS)} * DIMENSIONS);
    auto midOffset = (glm::ivec3{glm::lessThan(pos, HALF_DIMENSIONS) * glm::greaterThan(pos, -HALF_DIMENSIONS)} * HALF_DIMENSIONS);
    index = index + negOffset + midOffset;
        
    return index;
}

bool Game::Map::Map::Chunk::HasChanged()
{
    return hasChanged_;
}

void Game::Map::Map::Chunk::CommitChanges()
{
    hasChanged_ = false;
}

unsigned int Game::Map::Map::Chunk::GetBlockCount() const
{
    unsigned int blockCount = 0;
    for(int i = 0; i < Chunk::CHUNK_BLOCKS; i++)
        if(blocks_[i].type != Game::BlockType::NONE)
            blockCount++;

    return blockCount;
}

bool Game::Map::Map::Chunk::IsIndexValid(glm::ivec3 pos)
{
    return pos.x < DIMENSIONS.x && pos.y < DIMENSIONS.y && pos.z < DIMENSIONS.z &&
                pos.x >= 0 && pos.y >= 0 && pos.z >= 0;
}

bool Game::Map::Map::Chunk::IsPosValid(glm::ivec3 pos)
{
    auto index = pos + HALF_DIMENSIONS;
    return IsIndexValid(index);
}

int Game::Map::Map::Chunk::ToIndex(glm::ivec3 pos) const
{
    bool valid = IsIndexValid(pos);
    assertm(valid, "Invalid chunk pos");
    return pos.x + pos.y * CHUNK_WIDTH + pos.z * CHUNK_WIDTH * CHUNK_HEIGHT;
}

glm::ivec3 Game::Map::Map::Chunk::ToPos(uint32_t index) const
{
    auto z = index / (CHUNK_WIDTH * CHUNK_HEIGHT);
    auto xRemainder = index % (CHUNK_WIDTH * CHUNK_HEIGHT);
    auto y = xRemainder / (CHUNK_WIDTH);
    auto x = xRemainder % (CHUNK_WIDTH);

    return glm::ivec3{x, y, z};
}

// #### BlockIterator #### \\

std::pair<glm::ivec3, Game::Block const*> Game::Map::Map::Chunk::BlockIterator::GetNextBlock()
{   
    auto ret = std::pair{glm::ivec3{0}, (Game::Block const*)nullptr};

    auto block = &chunk_->blocks_[index_];
    while(block->type == BlockType::NONE && index_ < (CHUNK_BLOCKS - 1))
    {
        index_++;
        block = &chunk_->blocks_[index_];
    }

    if(index_ < CHUNK_BLOCKS)
    {
        ret.first = chunk_->ToPos(index_) - Chunk::HALF_DIMENSIONS;
        ret.second = block;
        index_++;
    }
    
    end_ = !ret.second || ret.second->type == BlockType::NONE;

    return ret;
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


// ### Blocks #### \\

glm::vec3 Game::Map::ToRealPos(glm::ivec3 globalPos, float blockScale)
{
    return glm::vec3{globalPos} * blockScale + blockScale / 2.0f;
}

glm::vec3 Game::Map::ToRealPos(glm::ivec3 chunkIndex, glm::ivec3 blockIndex, float blockScale)
{
    return glm::vec3{ToGlobalPos(chunkIndex, blockIndex)} * blockScale;
}

glm::ivec3 Game::Map::ToGlobalPos(glm::vec3 realPos, float blockScale)
{
    //return glm::round(realPos / blockScale);
    return glm::floor(realPos / blockScale);
}

glm::ivec3 Game::Map::ToGlobalPos(glm::ivec3 chunkIndex, glm::ivec3 blockIndex)
{
    return glm::vec3{chunkIndex} * glm::vec3{Game::Map::Map::Chunk::DIMENSIONS} + glm::vec3{blockIndex};
}

glm::ivec3 Game::Map::ToBlockChunkPos(glm::ivec3 blockPos)
{   
    auto centeredPos = blockPos + Map::Chunk::HALF_DIMENSIONS;
    auto mod = centeredPos % (Map::Chunk::DIMENSIONS);
    glm::ivec3 modSign = glm::lessThan(mod, glm::ivec3{0});

    auto blockChunkPos = mod + Map::Chunk::DIMENSIONS * modSign;
    return blockChunkPos;
}

// #### Chunks #### \\

glm::vec3 Game::Map::ToRealChunkPos(glm::ivec3 chunkIndex, float blockScale)
{
    return glm::vec3{chunkIndex} * glm::vec3{Game::Map::Map::Chunk::DIMENSIONS} * blockScale;
}

glm::ivec3 Game::Map::ToChunkIndex(glm::vec3 realPos, float blockScale)
{
    glm::vec3 pos = ToGlobalPos(realPos, blockScale) + Game::Map::Map::Chunk::HALF_DIMENSIONS;
    return glm::floor(pos / glm::vec3{Game::Map::Map::Chunk::DIMENSIONS});
}

glm::ivec3 Game::Map::ToChunkIndex(glm::ivec3 globalBlockpos)
{
    auto cPos = globalBlockpos + Map::Chunk::HALF_DIMENSIONS;
    glm::ivec3 sign = glm::lessThan(cPos, glm::ivec3{0});
    glm::ivec3 chunkIndex = (cPos + sign) / Map::Chunk::DIMENSIONS + sign * glm::ivec3{-1};
    return chunkIndex;
}