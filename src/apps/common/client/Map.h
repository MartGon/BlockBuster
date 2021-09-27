#pragma once

#include <game/Map.h>
#include <rendering/TexturePalette.h>
#include <rendering/ColorPalette.h>
#include <rendering/ChunkMeshMgr.h>
#include <util/Serializable.h>

namespace App::Client
{
    class Map : public Util::Serializable
    {
    public:

        Game::Block GetBlock(glm::ivec3 pos);
        void SetBlock(glm::ivec3 pos, Game::Block block);

        Util::Buffer ToBuffer();

    private:
        Game::Map::Map map_;
        Rendering::TexturePalette tPalette{16};
        Rendering::ColorPalette cPalette{32};
        std::filesystem::path textureFolder;
        Rendering::ChunkMesh::Manager chunkMeshMgr_{&map_};
    };
}