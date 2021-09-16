#include <rendering/ChunkMeshBuilder.h>

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