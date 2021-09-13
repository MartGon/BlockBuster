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
        void AddFace(FaceType face, glm::vec3 voxelPos, int displayType, int textureId, float blockScale = 1.0f);
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