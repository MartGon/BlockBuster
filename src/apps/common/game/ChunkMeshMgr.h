#pragma once

#include <unordered_map>

#include <ChunkMeshBuilder.h>
#include <entity/Map.h>

#include <iostream>


namespace Rendering::ChunkMesh
{
    class Manager
    {
    public:
        Manager() {}
        Manager(Game::Map::Map* map, float blockScale = 2.0f) : map_{map}, blockScale_{blockScale}
        {
        }

        void Reset()
        {
            chunkData_.clear();
        }

        void SetMap(Game::Map::Map* map)
        {
            map_ = map;
            Reset();
        }

        void SetBlockScale(float blockScale)
        {
            blockScale_ = blockScale;
            Reset();
        }

        void Update();
        void DrawChunks(GL::Shader& shader, const glm::mat4& view);

    private:

        bool ChunkFound(glm::ivec3 pos) const;

        struct ChunkData
        {
            glm::mat4 transform;
            Rendering::Mesh mesh;
        };

        Game::Map::Map* map_;
        std::unordered_map<glm::ivec3, ChunkData> chunkData_;
        float blockScale_ = 2.0f;
    };

}