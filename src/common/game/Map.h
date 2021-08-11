#pragma once

#include <Block.h>

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <unordered_map>

namespace Game
{
    class Map
    {
    public:

        class Chunk
        {
        public:
            Chunk();
            Game::Block* GetBlock(glm::ivec3 pos);
            void SetBlock(glm::ivec3 pos, Game::Block block);

            constexpr static const int CHUNK_WIDTH = 16;
            constexpr static const int CHUNK_HEIGHT = 16;
            constexpr static const int CHUNK_DEPTH = 16;
            constexpr static const int CHUNK_BLOCKS = CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_DEPTH;
            constexpr static const glm::ivec3 DIMENSIONS{CHUNK_WIDTH, CHUNK_HEIGHT, CHUNK_DEPTH};
            constexpr static const glm::ivec3 HALF_DIMENSIONS = (DIMENSIONS / 2);
        private:
            int ToIndex(glm::ivec3 pos);

            Block blocks_[CHUNK_BLOCKS];
        };

        Game::Block* GetBlock(glm::ivec3 pos);
        void AddBlock(glm::ivec3 pos, Game::Block block);

        std::vector<glm::ivec3> GetChunkIndices() const;
        Chunk& GetChunk(glm::ivec3 pos);

    private:

        glm::ivec3 ToChunkIndex(glm::ivec3 blockPos);
        glm::ivec3 ToBlockChunkPos(glm::ivec3 blockPos);

        glm::ivec3 dimensions_{1};
        std::unordered_map<glm::ivec3, Chunk> chunks_;
    };
}