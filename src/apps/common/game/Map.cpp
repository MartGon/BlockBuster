#include <game/Map.h>

#include <iostream>

using namespace App::Client;

Map::Map(Map&& other)
{
    *this = std::move(other);
}

Map& Map::operator=(Map&& other) 
{
    std::swap(this->map_, other.map_);
    std::swap(this->textureFolder, textureFolder);
    std::swap(this->blockScale, other.blockScale);
    std::swap(tPalette, other.tPalette);
    std::swap(cPalette, other.cPalette);

    // This is not moved, still points to the same location
    chunkMeshMgr_.Reset();
    chunkMeshMgr_.Update();
    
    return *this;
}

void Map::SetBlockScale(float scale)
{
    this->blockScale = scale;
}

float Map::GetBlockScale() const
{
    return blockScale;
}

Game::Map::Map* Map::GetMap()
{
    return &map_;
}


Game::Block Map::GetBlock(glm::ivec3 pos) const
{
    return *map_.GetBlock(pos);
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
    bool changed = false;
    for(auto pair : changedChunks)
        changed = changed || pair.second;

    return changed;
}

void Map::CommitChanges()
{
    for(auto& pair : changedChunks)
        pair.second = false;
}

Util::Buffer Map::ToBuffer()
{
    Util::Buffer buffer;

    // Concat inner buffers
    buffer.Write(blockScale);
    buffer.Append(map_.ToBuffer());
    buffer.WriteStr(textureFolder);
    buffer.Append(tPalette.ToBuffer());
    buffer.Append(cPalette.ToBuffer());

    return buffer;
}

Map Map::FromBuffer(Util::Buffer::Reader reader)
{
    Map map;

    map.blockScale = reader.Read<float>();
    map.map_ = Game::Map::Map::FromBuffer(reader);
    map.textureFolder = reader.ReadStr();
    map.tPalette = Rendering::TexturePalette::FromBuffer(reader, map.textureFolder);
    map.cPalette = Rendering::ColorPalette::FromBuffer(reader);

    return std::move(map);
}