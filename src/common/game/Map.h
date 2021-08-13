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

        Game::Block* GetBlock(glm::ivec3 pos);
        void AddBlock(glm::ivec3 pos, Game::Block block);
        void RemoveBlock(glm::ivec3 pos);

        class Iterator
        {
        friend class Map;
        public:
            std::pair<glm::ivec3, Game::Block*> GetNextBlock();
            bool IsOver() const;
        private:
            Iterator(Map* map, std::vector<glm::ivec3> chunkIndices) : 
                map_{map},chunkIndices_{chunkIndices}, end_{chunkIndices_.empty()}
            {

            }

            Map* map_ = nullptr;
            unsigned int chunkMetaIndex = 0;
            glm::ivec3 blockOffset_{0};
            std::vector<glm::ivec3> chunkIndices_;

            bool end_ = false;
        };

        Iterator CreateIterator();

        class Chunk
        {
        public:
            Chunk();
            Game::Block* GetBlock(glm::ivec3 pos);
            void SetBlock(glm::ivec3 pos, Game::Block block);

            class BlockIterator
            {
            friend class Chunk;
            public:
                std::pair<glm::ivec3, Game::Block*> GetNextBlock();
                bool IsOver() const;
            private:
                BlockIterator(Chunk* chunk) : chunk_{chunk}
                {

                }

                Chunk* chunk_ = nullptr;
                glm::ivec3 index_{0};

                bool end_ = false;
            };

            BlockIterator CreateBlockIterator();

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

        class ChunkIterator
        {
        friend class Map;
        public:
            std::pair<glm::ivec3, Chunk*> GetNextChunk();
            bool IsOver() const;

        private:
            ChunkIterator(Map* map, std::vector<glm::ivec3> chunkIndices) :
                map_{map}, chunkIndices_{chunkIndices}, end_{chunkIndices_.empty()}
            {
                
            }

            Map* map_ = nullptr;
            int metaIndex_ = 0;
            std::vector<glm::ivec3> chunkIndices_;

            bool end_ = false;
        };

        ChunkIterator CreateChunkIterator();

    private:

        std::vector<glm::ivec3> GetChunkIndices() const;
        Chunk& GetChunk(glm::ivec3 pos);

        glm::ivec3 ToChunkIndex(glm::ivec3 blockPos);
        glm::ivec3 ToBlockChunkPos(glm::ivec3 blockPos);

        std::unordered_map<glm::ivec3, Chunk> chunks_;
    };
}