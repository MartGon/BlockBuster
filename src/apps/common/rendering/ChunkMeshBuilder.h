#pragma once

#include <glm/glm.hpp>

#include <rendering/Mesh.h>
#include <gl/VertexArray.h>
#include <gl/Shader.h>

#include <game/Block.h>

namespace Rendering::ChunkMesh
{
    enum FaceType
    {
        TOP = 0,
        BOTTOM = 1,
        RIGHT = 2,
        LEFT = 3,
        FRONT = 4,
        BACK = 5
    };

    class Builder
    {
    public:

        void Reset();
        void AddFace(FaceType face, glm::vec3 voxelPos, int displayType, int textureId, float blockScale = 1.0f);
        void AddSlopeFace(FaceType face, glm::vec3 voxelPos, Game::BlockRot rot, int displayType, int textureId, float blockScale = 1.0f);
        Mesh Build();
    private:
        std::vector<glm::vec3> vertices;
        // Note: Vertices have to be floats, otherwise some weird shit happens during rendering
        std::vector<int> vertexIndices;
        std::vector<unsigned int> indices;
        std::vector<glm::ivec2> displayInfo;
        int lastIndex = 0;
    };


}