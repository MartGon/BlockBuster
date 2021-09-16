#include <rendering/ChunkMeshMgr.h>

#include <debug/Debug.h>

using namespace Rendering;

const std::unordered_map<ChunkMesh::FaceType, glm::ivec3> offsets = {
    { ChunkMesh::FaceType::LEFT, glm::ivec3{-1, 0, 0} },
    { ChunkMesh::FaceType::RIGHT, glm::ivec3{1, 0, 0} },
    { ChunkMesh::FaceType::BOTTOM, glm::ivec3{0, -1, 0} },
    { ChunkMesh::FaceType::BACK, glm::ivec3{0, 0, -1} },
    { ChunkMesh::FaceType::TOP, glm::ivec3{0, 1, 0} },
    { ChunkMesh::FaceType::FRONT, glm::ivec3{0, 0, 1} },
};

const std::unordered_map<glm::ivec3, ChunkMesh::FaceType> revOffsets = {
    { glm::ivec3{-1, 0, 0}, ChunkMesh::FaceType::LEFT },
    { glm::ivec3{1, 0, 0}, ChunkMesh::FaceType::RIGHT },
    { glm::ivec3{0, -1, 0}, ChunkMesh::FaceType::BOTTOM },
    { glm::ivec3{0, 1, 0}, ChunkMesh::FaceType::TOP },
    { glm::ivec3{0, 0, -1}, ChunkMesh::FaceType::BACK },
    { glm::ivec3{0, 0, 1}, ChunkMesh::FaceType::FRONT },
};

glm::mat4 GetRotationMat(Game::Block const* nei)
{
    auto rot = nei->rot;
    auto t = Game::GetBlockTransform(glm::vec3{0}, rot, 1);
    return t.GetRotationMat();    
}

glm::mat4 GetInvRotationMat(Game::Block const* nei)
{
    return glm::inverse(GetRotationMat(nei));
}

glm::vec3 RotateOffset(const glm::mat4& rotMat, glm::vec3 offset)
{
    return glm::round(rotMat * glm::vec4{offset, 1.0f});
}

ChunkMesh::FaceType GetBorderFaceSlope(glm::ivec3 rotSide, const glm::mat4& rotMat)
{
    glm::ivec3 side = RotateOffset(rotMat, rotSide);
    auto it = revOffsets.find(side);
    assertm(it != revOffsets.end(), "Could not find side in revOffsets");

    return it->second;
}

Game::Block const* GetNeiBlock(const Game::Map::Map::Chunk& chunk, glm::ivec3 pos, glm::ivec3 offset)
{
    auto adjacentPos = pos + offset;
    Game::Block const* nei = nullptr;
    if(chunk.IsPosValid(adjacentPos))            
        nei = chunk.GetBlock(adjacentPos);

    return nei;
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
                auto nei = GetNeiBlock(chunk, pos, offset);

                if(!nei || nei->type == Game::BlockType::NONE)
                {
                    // This block has no neighbor in this side, build face
                    builder.AddFace(pair.first, pos, block->display.type, block->display.id);
                }
                else if(nei->type == Game::BlockType::SLOPE)
                {
                    auto rotMat = GetInvRotationMat(nei);
                    auto face = GetBorderFaceSlope(-offset, rotMat);
                    if(face != FaceType::BOTTOM && face != FaceType::BACK)
                        builder.AddFace(pair.first, pos, block->display.type, block->display.id);
                }
            }
        }
        else if(block->type == Game::BlockType::SLOPE)
        {
            // Slope face has to be always build. Can only be avoided if up, front, left and right are all covered.
            auto rotMat = GetInvRotationMat(block);
            std::unordered_map<FaceType, bool> neiFound;
            for(auto pair : offsets)   
            {
                auto offset = pair.second;
                auto face = GetBorderFaceSlope(offset, rotMat);

                auto nei = GetNeiBlock(chunk, pos, offset);
                if(!nei || nei->type == Game::BlockType::NONE)
                {
                    neiFound[face] = false;
                }
                else if(nei->type == Game::BlockType::BLOCK)
                {
                    neiFound[face] = true;
                }
                else if(nei->type == Game::BlockType::SLOPE)
                {
                    auto rotMat = GetInvRotationMat(nei);
                    auto neiFace = GetBorderFaceSlope(-offset, rotMat);
                    neiFound[face] = neiFace == FaceType::BOTTOM || neiFace == FaceType::BACK;
                }
            }

            const std::array<FaceType, 4> facesToCheck = {FaceType::TOP, FaceType::FRONT, FaceType::LEFT, FaceType::RIGHT};
            bool skipSlope = true;
            for(auto face : facesToCheck)
                skipSlope = skipSlope && neiFound[face];

            const std::array<FaceType, 4> facesToDraw = {FaceType::RIGHT, FaceType::LEFT, FaceType::BOTTOM, FaceType::BACK};
            for(auto face : facesToDraw)
            {   
                if(!neiFound[face])
                    builder.AddSlopeFace(face, pos, block->rot, block->display.type, block->display.id);
                else
                    std::cout << "Nei not found at " << face << "\n";
            }

            if(!skipSlope)
                builder.AddSlopeFace(FaceType::TOP, pos, block->rot, block->display.type, block->display.id);

            std::cout << "SkipSlope " << skipSlope << "\n";
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