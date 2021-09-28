#include <client/Map.h>

using namespace App::Client;

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

void Map::Draw(GL::Shader& shader, const glm::mat4& view)
{
    if(MapChanged())
    {
        chunkMeshMgr_.Update();
        CommitChanges();
    }

    chunkMeshMgr_.DrawChunks(shader, view);
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
    buffer.Append(map_.ToBuffer());
    buffer.Write(textureFolder);
    buffer.Append(tPalette.ToBuffer());
    buffer.Append(cPalette.ToBuffer());

    return buffer;
}