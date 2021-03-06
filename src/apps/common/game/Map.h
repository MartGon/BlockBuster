#pragma once

#include <entity/Map.h>
#include <TexturePalette.h>
#include <ColorPalette.h>
#include <ChunkMeshMgr.h>
#include <util/Buffer.h>

#include <filesystem>

namespace App::Client
{
    class Map
    {
    public:
        Map() = default;

        Map(Map&& other);// = default;
        Map& operator=(Map&& other);// = default;

        Map(const Map& other) = delete;
        Map& operator=(Map& other) = delete;

        ~Map() = default;

        void SetBlockScale(float scale);
        float GetBlockScale() const;

        Game::Map::Map* GetMap();
        Game::Block GetBlock(glm::ivec3 pos) const;
        void SetBlock(glm::ivec3 pos, Game::Block block);
        bool IsNullBlock(glm::ivec3 pos) const;
        bool CanPlaceBlock(glm::ivec3 pos) const;
        void RemoveBlock(glm::ivec3 pos);
        uint32_t GetBlockCount() const;

        Util::Buffer ToBuffer();
        static Map FromBuffer(Util::Buffer::Reader reader, std::filesystem::path mapFolder);

        void Draw(GL::Shader& shader, const glm::mat4& view);
        void DrawChunkBorders(GL::Shader& shader, Rendering::Mesh& cubeMesh, const glm::mat4& view, glm::vec4 color = glm::vec4{1.0f, 1.0f, 0.0f, 1.0f});

        Rendering::TexturePalette tPalette{16};
        Rendering::ColorPalette cPalette{32};

    private:

        bool MapChanged() const;
        void CommitChanges();

        bool changed = false;
        Game::Map::Map map_;

        Rendering::ChunkMesh::Manager chunkMeshMgr_{&map_};
        std::unordered_map<glm::ivec3, bool> changedChunks;
    };
}