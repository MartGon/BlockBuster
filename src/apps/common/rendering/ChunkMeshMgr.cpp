#include <rendering/ChunkMeshMgr.h>

using namespace Rendering;

void ChunkMesh::Manager::Update()
{
    auto cIt = map_->CreateChunkIterator();
    for(auto cData = cIt.GetNextChunk(); !cIt.IsOver(); cData = cIt.GetNextChunk())
    {
        auto chunk = cData.second;
        if(chunk->HasChanged())
        {
            auto chunkIndex = cData.first;
            meshes_[chunkIndex] = GenerateChunkMesh(map_, chunkIndex);
            chunk->CommitChanges();
        }
    }
}

const std::unordered_map<glm::ivec3, Rendering::Mesh>& ChunkMesh::Manager::GetMeshes()
{
    return meshes_;
}

