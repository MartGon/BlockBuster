#include <rendering/ChunkMesh.h>

#include <array>


using namespace Rendering;

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
        std::array<glm::vec3, 4>{ glm::vec3{-1, -1, -1}, {1, -1, -1}, {-1, -1, 1}, {1, -1, 1} }, // UP

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