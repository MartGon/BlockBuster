#pragma once

#include <unordered_map>

#include <rendering/ChunkMeshBuilder.h>
#include <game/Map.h>

namespace Rendering::ChunkMesh
{
    Rendering::Mesh GenerateChunkMesh(Game::Map::Map* map, glm::ivec3 chunkIndex);

    class Manager
    {
    public:
        Manager() {}
        Manager(Game::Map::Map* map) : map_{map}
        {

        }

        void Reset()
        {
            meshes_.clear();
        }

        void SetMap(Game::Map::Map* map)
        {
            map_ = map;
            meshes_.clear();
        }

        void Update();
        const std::unordered_map<glm::ivec3, Rendering::Mesh>& GetMeshes();

    private:
        Game::Map::Map* map_;
        std::unordered_map<glm::ivec3, Rendering::Mesh> meshes_;
    };

}