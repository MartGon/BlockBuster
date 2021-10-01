#include <ChunkMeshMgr.h>

#include <debug/Debug.h>

using namespace Rendering;

void ChunkMesh::Manager::Update()
{
    bool init = chunkData_.empty();

    auto cIt = map_->CreateChunkIterator();
    for(auto cData = cIt.GetNextChunk(); !cIt.IsOver(); cData = cIt.GetNextChunk())
    {
        auto chunk = cData.second;
        if(chunk->HasChanged() || init)
        {
            auto chunkIndex = cData.first;

            ChunkData cd;
            if(!ChunkFound(chunkIndex))
            {
                // Calculate transform mat
                auto pos = Game::Map::ToRealChunkPos(chunkIndex, blockScale_);
                Math::Transform t{pos, glm::vec3{0.0f}, glm::vec3{blockScale_ / 2.0f}};
                cd.transform = t.GetTransformMat();
            }
            else
                cd = std::move(chunkData_[chunkIndex]);

            cd.mesh = GenerateChunkMesh(map_, chunkIndex);
            chunkData_[chunkIndex] = std::move(cd);
            chunk->CommitChanges();
        }
    }
}

void ChunkMesh::Manager::DrawChunks(GL::Shader& shader, const glm::mat4& view)
{
    for(auto& pair : chunkData_)
    {
        auto& data = pair.second;
        auto tMat = view * data.transform;
        shader.SetUniformMat4("transform", tMat);
        data.mesh.Draw(shader);
    }
}

bool ChunkMesh::Manager::ChunkFound(glm::ivec3 chunkIndex) const
{
    return chunkData_.find(chunkIndex) != chunkData_.end();
}

