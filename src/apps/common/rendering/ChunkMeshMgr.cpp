#include <rendering/ChunkMeshMgr.h>

using namespace Rendering;

const std::unordered_map<ChunkMesh::FaceType, glm::ivec3> offsets = {
    { ChunkMesh::FaceType::LEFT, glm::ivec3{-1, 0, 0} },
    { ChunkMesh::FaceType::RIGHT, glm::ivec3{1, 0, 0} },
    { ChunkMesh::FaceType::BOTTOM, glm::ivec3{0, -1, 0} },
    { ChunkMesh::FaceType::TOP, glm::ivec3{0, 1, 0} },
    { ChunkMesh::FaceType::BACK, glm::ivec3{0, 0, -1} },
    { ChunkMesh::FaceType::FRONT, glm::ivec3{0, 0, 1} },
};

ChunkMesh::FaceType GetBorderFaceSlope(glm::ivec3 offset, Game::Block const* nei)
{

}

// TODO: Should check the map instead of only within chunk neighbors
Mesh ChunkMesh::GenerateChunkMesh(Game::Map::Map* map, glm::ivec3 chunkIndex)
{
    ChunkMesh::Builder builder;

    auto& chunk = map->GetChunk(chunkIndex);
    auto bIt = chunk.CreateBlockIterator();
    for(auto bData = bIt.GetNextBlock(); !bIt.IsOver(); bData = bIt.GetNextBlock())
    {
        auto pos = bData.first;
        auto block = bData.second;
        if(block->type == Game::BlockType::BLOCK)
        {
            for(auto pair : offsets)   
            {
                auto offset = pair.second;
                auto adjacentPos = pos + offset;
                Game::Block const* nei = nullptr;
                if(!chunk.IsPosValid(adjacentPos))            
                    nei = chunk.GetBlock(adjacentPos);

                if(!nei || nei->type == Game::BlockType::NONE)
                {
                    // This block has no neighbor in this side, build face
                    builder.AddFace(pair.first, pos, block->display.type, block->display.id);
                }
                else if(nei->type == Game::BlockType::SLOPE)
                {
                    auto face = GetBorderFaceSlope(offset, nei);
                    //if(face == )
                }
            }
        }
        else if(block->type == Game::BlockType::SLOPE)
        {
            // Slope face has to be always build. Can only be avoided if up, front, left and right are all covered.
            // Do it for base rotation, then transform iterated offsets.
            for(auto pair : offsets)   
            {
                builder.AddSlopeFace(pair.first, pos, block->rot, block->display.type, block->display.id);
            }
        }
    }

    return builder.Build();
}

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