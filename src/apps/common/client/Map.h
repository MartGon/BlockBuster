#pragma once

#include <game/Map.h>
#include <rendering/TexturePalette.h>
#include <rendering/ColorPalette.h>
#include <rendering/ChunkMeshMgr.h>
#include <util/Serializable.h>

namespace App::Client
{
    class Map
    {
    public:
        Game::Block GetBlock(glm::ivec3 pos) const;
        void SetBlock(glm::ivec3 pos, Game::Block block);

        Util::Buffer ToBuffer();

        void Draw(GL::Shader& shader, const glm::mat4& view);

    private:

        bool MapChanged() const;
        void CommitChanges();

        Game::Map::Map map_;
        Rendering::TexturePalette tPalette{16};
        Rendering::ColorPalette cPalette{32};
        std::filesystem::path textureFolder;

        Rendering::ChunkMesh::Manager chunkMeshMgr_{&map_};
        std::unordered_map<glm::ivec3, bool> changedChunks;
    };
}