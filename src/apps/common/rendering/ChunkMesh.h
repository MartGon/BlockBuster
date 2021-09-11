#pragma once

#include <glm/glm.hpp>

#include <rendering/Mesh.h>
#include <gl/VertexArray.h>
#include <gl/Shader.h>

namespace Rendering::ChunkMesh
{
    enum FaceType
    {
        UP,
        DOWN,
        RIGHT,
        LEFT,
        FRONT,
        BACK
    };

    class Builder
    {
    public:

        void Reset();
        void AddFace(FaceType face, glm::vec3 voxelPos, int displayType, int textureId);
        Mesh Build();

        void Draw();
    private:
        std::vector<glm::vec3> vertices;
        std::vector<int> vertexIndices;
        std::vector<unsigned int> indices;
        std::vector<glm::ivec2> displayInfo;
        int lastIndex = 0;
    };


}