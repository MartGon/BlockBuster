#include <ChunkMeshBuilder.h>

#include <math/BBMath.h>

#include <array>

#include <debug/Debug.h>

using namespace Rendering;

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
            builder.AddBlockMesh(chunk, pos, block);
        }
        else if(block->type == Game::BlockType::SLOPE)
        {
            builder.AddSlopeMesh(chunk, pos, block);
        }
    }

    return builder.Build();
}

void ChunkMesh::Builder::Reset()
{
    vertices.clear();
    vertexIndices.clear();
    displayInfo.clear();
    indices.clear();

    lastIndex = 0;
}

void ChunkMesh::Builder::AddFace(FaceType face, glm::vec3 voxelPos, int displayType, int textureId, float blockScale)
{
    static const std::array<std::array<glm::vec3, 4>, 6> FACES = {
        std::array<glm::vec3, 4>{ glm::vec3{-1, 1, -1}, {-1, 1, 1}, {1, 1, -1}, {1, 1, 1} }, // TOP
        std::array<glm::vec3, 4>{ glm::vec3{-1, -1, -1}, {1, -1, -1}, {-1, -1, 1}, {1, -1, 1} }, // BOTTOM

        std::array<glm::vec3, 4>{ glm::vec3{1, -1, -1}, {1, 1, -1}, {1, -1, 1}, {1, 1, 1} }, // RIGHT
        std::array<glm::vec3, 4>{ glm::vec3{-1, -1, -1}, {-1, -1, 1}, {-1, 1, -1}, {-1, 1, 1} }, // LEFT

        std::array<glm::vec3, 4>{ glm::vec3{-1, 1, 1}, {-1, -1, 1}, {1, 1, 1}, {1, -1, 1} }, // FRONT
        std::array<glm::vec3, 4>{ glm::vec3{-1, 1, -1}, {1, 1, -1}, {-1, -1, -1}, {1, -1, -1} }, // BACK
    };

    // Push vertex
    auto faceVerts = FACES[face];
    for(auto i = 0; i < 4; i++)
    {
        auto vertex = faceVerts[i] + (voxelPos + glm::vec3{0.5f}) * blockScale * 2.0f;
        vertices.push_back(vertex);

        // TODO: Push normals / Push face index and Get normal from shader
        
        vertexIndices.push_back(i);

        displayInfo.push_back({displayType, textureId});
    }

    // Push indices
    indices.push_back(lastIndex);
    indices.push_back(lastIndex + 1);
    indices.push_back(lastIndex + 2);
    indices.push_back(lastIndex + 3);
    indices.push_back(lastIndex + 2);
    indices.push_back(lastIndex + 1);
    lastIndex += 4;
}

void ChunkMesh::Builder::AddSlopeFace(FaceType face, glm::vec3 voxelPos, Game::BlockRot rot, int displayType, int textureId, float blockScale)
{
    static const std::array<std::vector<glm::vec3>, 6> FACES = {
        std::vector<glm::vec3>{ glm::vec3{-1, -1, 1}, {1, -1, 1}, {-1, 1, -1}, {1, 1, -1} }, // TOP
        std::vector<glm::vec3>{ glm::vec3{-1, -1, -1}, {1, -1, -1}, {-1, -1, 1}, {1, -1, 1} }, // BOTTOM

        std::vector<glm::vec3>{ glm::vec3{1, -1, -1}, {1, 1, -1}, {1, -1, 1} }, // RIGHT
        std::vector<glm::vec3>{ glm::vec3{-1, -1, -1}, {-1, -1, 1}, {-1, 1, -1} }, // LEFT

        std::vector<glm::vec3>{ glm::vec3{-1, -1, 1}, {1, -1, 1}, {-1, 1, -1}, {1, 1, -1} }, // FRONT
        std::vector<glm::vec3>{ glm::vec3{-1, 1, -1}, {1, 1, -1}, {-1, -1, -1}, {1, -1, -1} }, // BACK
    };

    // Push vertex
    auto faceVerts = FACES[face];
    auto vertsSize = faceVerts.size();
    for(auto i = 0; i < vertsSize; i++)
    {
        // Rotate offset
        auto t = Game::GetBlockTransform(glm::ivec3{0}, rot, 1.0f);
        glm::vec3 offset = t.GetRotationMat() * glm::vec4{faceVerts[i], 1};
        
        auto vertex = offset + (voxelPos + glm::vec3{0.5f}) * blockScale * 2.0f;
        
        vertices.push_back(vertex);

        // TODO: Push normals / Push face index and Get normal from shader
        
        vertexIndices.push_back(i);

        displayInfo.push_back({displayType, textureId});
    }

    // Push indices
    indices.push_back(lastIndex);
    indices.push_back(lastIndex + 1);
    indices.push_back(lastIndex + 2);
    if(vertsSize == 4)
    {
        indices.push_back(lastIndex + 3);
        indices.push_back(lastIndex + 2);
        indices.push_back(lastIndex + 1);
    }
    lastIndex += vertsSize;
}

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

void ChunkMesh::Builder::AddBlockMesh(const Game::Map::Map::Chunk& chunk, glm::ivec3 pos, Game::Block const* block)
{
    for(auto pair : offsets)   
    {
        auto offset = pair.second;
        auto nei = GetNeiBlock(chunk, pos, offset);

        if(!nei || nei->type == Game::BlockType::NONE)
        {
            AddFace(pair.first, pos, block->display.type, block->display.id);
        }
        else if(nei->type == Game::BlockType::SLOPE)
        {
            auto rotMat = Math::GetRotMatInverse(nei->GetRotationMat());
            auto face = GetBorderFaceSlope(-offset, rotMat);
            if(face != FaceType::BOTTOM && face != FaceType::BACK)
                AddFace(pair.first, pos, block->display.type, block->display.id);
        }
    }
}

void ChunkMesh::Builder::AddSlopeMesh(const Game::Map::Map::Chunk& chunk, glm::ivec3 pos, Game::Block const* block)
{
    struct SkipInfo
    {
        bool skip = false;
        bool fullFace = false;
    };

    auto rotMat = Math::GetRotMatInverse(block->GetRotationMat());
    std::unordered_map<FaceType, SkipInfo> skipDraw;
    skipDraw.reserve(6);
    for(auto pair : offsets)   
    {
        auto offset = pair.second;
        auto face = GetBorderFaceSlope(offset, rotMat);

        auto nei = GetNeiBlock(chunk, pos, offset);
        if(!nei || nei->type == Game::BlockType::NONE)
            skipDraw[face] = SkipInfo{false, false};
        else if(nei->type == Game::BlockType::BLOCK)
            skipDraw[face] = SkipInfo{true, true};
        else if(nei->type == Game::BlockType::SLOPE)
        {
            auto rotMat = Math::GetRotMatInverse(nei->GetRotationMat());
            auto neiFace = GetBorderFaceSlope(-offset, rotMat);
            bool sameSide = neiFace == FaceType::LEFT && face == FaceType::RIGHT || neiFace == FaceType::RIGHT && face == FaceType::LEFT;
            bool sameRot = nei->rot == block->rot;
            bool fullFace = neiFace == FaceType::BOTTOM || neiFace == FaceType::BACK;
            bool skip = fullFace || (sameSide && sameRot);
            skipDraw[face] = SkipInfo{skip, fullFace};
        }
    }

    const std::array<FaceType, 4> facesToDraw = {FaceType::RIGHT, FaceType::LEFT, FaceType::BOTTOM, FaceType::BACK};
    for(auto face : facesToDraw)
    {   
        if(!skipDraw[face].skip)
            AddSlopeFace(face, pos, block->rot, block->display.type, block->display.id);
    }

    // Slope face has to be build most of the times. Can only be avoided if up, front, left and right are all covered by full faces
    bool skipSlope = skipDraw[FaceType::TOP].fullFace && skipDraw[FaceType::FRONT].fullFace && 
        skipDraw[FaceType::LEFT].fullFace && skipDraw[FaceType::RIGHT].fullFace;
    if(!skipSlope)
        AddSlopeFace(FaceType::TOP, pos, block->rot, block->display.type, block->display.id);
}

Mesh ChunkMesh::Builder::Build()
{
    Mesh mesh;

    auto& vao = mesh.GetVAO();

    vao.GenVBO(vertices, 3);
    vao.GenVBO(displayInfo, 2);
    vao.GenVBO(vertexIndices, 1);
    vao.SetIndices(indices);

    return mesh;
}

// #### PRIVATE #### \\

ChunkMesh::FaceType ChunkMesh::Builder::GetBorderFaceSlope(glm::ivec3 rotSide, const glm::mat4& rotMat)
{
    glm::ivec3 side = Math::RotateVec3(rotMat, rotSide);
    auto it = revOffsets.find(side);
    assertm(it != revOffsets.end(), "Could not find side in revOffsets");

    return it->second;
}

Game::Block const* ChunkMesh::Builder::GetNeiBlock(const Game::Map::Map::Chunk& chunk, glm::ivec3 pos, glm::ivec3 offset)
{
    auto adjacentPos = pos + offset;
    Game::Block const* nei = nullptr;
    if(chunk.IsPosValid(adjacentPos))            
        nei = chunk.GetBlock(adjacentPos);

    return nei;
}