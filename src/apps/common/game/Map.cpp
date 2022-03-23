#include <game/Map.h>

#include <ServiceLocator.h>

#include <iostream>

using namespace App::Client;

Map::Map(Map&& other)
{
    *this = std::move(other);
}

Map& Map::operator=(Map&& other) 
{
    std::swap(this->map_, other.map_);
    std::swap(tPalette, other.tPalette);
    std::swap(cPalette, other.cPalette);

    // This is not moved, still points to the same location
    chunkMeshMgr_.SetBlockScale(this->map_.GetBlockScale());
    chunkMeshMgr_.Reset();
    chunkMeshMgr_.Update();
    
    return *this;
}

void Map::SetBlockScale(float scale)
{
    this->changed = true;

    chunkMeshMgr_.SetBlockScale(scale);
}

float Map::GetBlockScale() const
{
    return map_.GetBlockScale();
}

Game::Map::Map* Map::GetMap()
{
    return &map_;
}

Game::Block Map::GetBlock(glm::ivec3 pos) const
{
    Game::Block block{Game::BlockType::NONE};
    if(auto found = map_.GetBlock(pos))
        block = *found;
    return block;
}

void Map::SetBlock(glm::ivec3 pos, Game::Block block)
{
    auto chunkIndex = Game::Map::ToChunkIndex(pos);
    changedChunks[chunkIndex] = true;
    map_.AddBlock(pos, block);
}

bool Map::IsNullBlock(glm::ivec3 pos) const
{
    return map_.IsNullBlock(pos);
}

bool Map::CanPlaceBlock(glm::ivec3 pos) const
{
    auto rids = map_.GetGameObjectIndices();
    bool found = std::find(rids.begin(), rids.end(), pos) != rids.end();
    bool isNull = IsNullBlock(pos);

    return !found && isNull;
}

void Map::RemoveBlock(glm::ivec3 pos)
{
    auto chunkIndex = Game::Map::ToChunkIndex(pos);
    changedChunks[chunkIndex] = true;
    map_.RemoveBlock(pos);
}

uint32_t Map::GetBlockCount() const
{
    return map_.GetBlockCount();
}

void Map::Draw(GL::Shader& shader, const glm::mat4& view)
{
    if(MapChanged())
    {
        chunkMeshMgr_.Update();
        CommitChanges();
    }

    chunkMeshMgr_.DrawChunks(shader, view);
}

void Map::DrawChunkBorders(GL::Shader& shader, Rendering::Mesh& cubeMesh, const glm::mat4& view, glm::vec4 color)
{
    auto chunkIt = map_.CreateChunkIterator();
    for(auto chunk = chunkIt.GetNextChunk(); !chunkIt.IsOver(); chunk = chunkIt.GetNextChunk())
    {
        auto blockScale = map_.GetBlockScale();
        auto pos = Game::Map::ToRealChunkPos(chunk.first, blockScale);
        auto size = glm::vec3{Game::Map::Map::Chunk::DIMENSIONS} * blockScale;
        Math::Transform ct{pos, glm::vec3{0.0f}, size};
        auto model = ct.GetTransformMat();

        auto transform = view * model;
        shader.SetUniformMat4("transform", transform);
        shader.SetUniformInt("hasBorder", false);
        shader.SetUniformInt("overrideColor", true);
        shader.SetUniformInt("textureType", 1);
        shader.SetUniformVec4("color", color);
        glDisable(GL_CULL_FACE);
        cubeMesh.Draw(shader, GL_LINE);
        glEnable(GL_CULL_FACE);
    }
}

bool Map::MapChanged() const
{
    auto changed = this->changed;
    for(auto pair : changedChunks)
        changed = changed || pair.second;

    return changed;
}

void Map::CommitChanges()
{
    for(auto& pair : changedChunks)
        pair.second = false;
    changed = false;
}

Util::Buffer Map::ToBuffer()
{
    Util::Buffer buffer;

    // Concat inner buffers
    buffer.Append(map_.ToBuffer());
    buffer.Append(tPalette.ToBuffer());
    buffer.Append(cPalette.ToBuffer());

    return buffer;
}

Map Map::FromBuffer(Util::Buffer::Reader reader, std::filesystem::path mapFolder)
{
    Map map;

    map.map_ = Game::Map::Map::FromBuffer(reader);
    //reader.Read<std::string>(); // TODO: Remove
    auto textureFolder = mapFolder / "textures";
    map.tPalette = Rendering::TexturePalette::FromBuffer(reader, textureFolder);
    map.cPalette = Rendering::ColorPalette::FromBuffer(reader);

    return std::move(map);
}