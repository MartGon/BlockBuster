#pragma once

#include <glm/glm.hpp>

#include <rendering/Mesh.h>
#include <gl/VertexArray.h>
#include <gl/Shader.h>

#include <entity/Map.h>

namespace Rendering::ChunkMesh
{
    Rendering::Mesh GenerateChunkMesh(Game::Map::Map* map, glm::ivec3 chunkIndex);

    enum FaceType
    {
        TOP = 0,
        BOTTOM = 1,
        RIGHT = 2,
        LEFT = 3,
        FRONT = 4,
        BACK = 5,
        END = 6
    };

    class Builder
    {
    public:

        void Reset();
        void AddFace(FaceType face, glm::vec3 voxelPos, int displayType, int textureId, float blockScale = 1.0f);
        void AddSlopeFace(FaceType face, glm::vec3 voxelPos, Game::BlockRot rot, int displayType, int textureId, float blockScale = 1.0f);

        void AddBlockMesh(const Game::Map::Map::Chunk& chunk, glm::ivec3 pos, Game::Block const* block);
        void AddSlopeMesh(const Game::Map::Map::Chunk& chunk, glm::ivec3 pos, Game::Block const* slope);

        Mesh Build();
    private:

        ChunkMesh::FaceType GetBorderFaceSlope(glm::ivec3 rotSide, const glm::mat4& rotMat);
        Game::Block const* GetNeiBlock(const Game::Map::Map::Chunk& chunk, glm::ivec3 pos, glm::ivec3 offset);

        std::vector<glm::vec3> vertices;
        // Note: Vertices have to be floats, otherwise some weird shit happens during rendering
        std::vector<int> vertexIndices;
        std::vector<unsigned int> indices;
        std::vector<glm::ivec2> displayInfo;
        int lastIndex = 0;
    };


}