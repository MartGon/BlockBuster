#pragma once

#include <game/Block.h>

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <unordered_map>

namespace Game
{
    namespace Map
    {
        class Map
        {
        public:
            class Chunk;

            Map() = default;

            Map(Map& other) = delete;
            Map& operator=(const Map& other) = delete;

            Map(Map&&) = default;
            Map& operator=(Map&&) = default;

            // Gets a block by its global position
            Game::Block const* GetBlock(glm::ivec3 pos);
            // Sets a block by its global position
            void AddBlock(glm::ivec3 pos, Game::Block block);
            // Removes a block by its global position
            void RemoveBlock(glm::ivec3 pos);
            // Checks a block by its global position
            bool IsBlockNull(glm::ivec3 pos);

            std::vector<glm::ivec3> GetChunkIndices() const;
            // Gets a chunk by its chunk index
            Chunk& GetChunk(glm::ivec3 pos);
            void Clear();

            unsigned int GetBlockCount() const;

            class Iterator
            {
            friend class Map;
            public:
                // Returns global block pos
                std::pair<glm::ivec3, Game::Block const*> GetNextBlock();
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

                Chunk(const Chunk&) = delete;
                Chunk& operator=(const Chunk&) = delete;

                Chunk(Chunk&&) = default;
                Chunk& operator=(Chunk&&) = default;

                // Gets a block by its internal index in the chunk [0, 15]
                Game::Block const* GetBlockByIndex(glm::ivec3 pos);
                // Sets a block by its internal index in the chunk [0, 15]
                void SetBlockByIndex(glm::ivec3 pos, Game::Block block);

                // Gets a block by its position within the chunk [-8, 7]
                Game::Block const* GetBlock(glm::ivec3 pos);
                // Sets a block by its position within the chunk [-8, 7]
                void SetBlock(glm::ivec3 pos, Game::Block block);

                bool HasChanged();
                void CommitChanges();

                unsigned int GetBlockCount() const;

                static bool IsIndexValid(glm::ivec3 pos);
                static bool IsPosValid(glm::ivec3 pos);

                class BlockIterator
                {
                friend class Chunk;
                public:
                    // Returns block pos in chunk [-8, 7]
                    std::pair<glm::ivec3, Game::Block const*> GetNextBlock();
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
                bool hasChanged_ = false;

                Block blocks_[CHUNK_BLOCKS];
            };

            class ChunkIterator
            {
            friend class Map;
            public:
                // Returns chunk index 
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
            std::unordered_map<glm::ivec3, Chunk> chunks_;
        };

        // Position transforms

        // #### Blocks #### \\
        // GlobalPos: ivec3 coordinates of a block in the map
        // ChunkPos: ivec3 coordinates of a block relative to a chunk
        // RealPos: vec3 position of a block/chunk taking block scale into account

        // Returns real block pos from global block pos
        glm::vec3 ToRealPos(glm::ivec3 globalPos, float blockScale = 1.0f);

        // Returns real block pos from chunk index and block index within that chunk.
        glm::vec3 ToRealPos(glm::ivec3 chunkIndex, glm::ivec3 blockIndex, float blockScale = 1.0f);

        // Returns the block corresponding to a real pos;
        glm::ivec3 ToGlobalPos(glm::vec3 globalPos, float blockScale = 1.0f);

        // Returns global block pos from chunk index and block index within that chunk. Ignores scale
        glm::ivec3 ToGlobalPos(glm::ivec3 chunkIndex, glm::ivec3 blockIndex);

        // Global pos to position within a chunk
        glm::ivec3 ToBlockChunkPos(glm::ivec3 blockPos);

        // #### Chunks #### \\

        // Returns centered chunk pos
        glm::vec3 ToRealChunkPos(glm::ivec3 chunkIndex, float blockScale = 1.0f);

        // Returns the chunk index corresponding to a given real pos
        glm::ivec3 ToChunkIndex(glm::vec3 realPos, float blockScale = 1.0f);

        // Global pos to corresponding chunk index
        glm::ivec3 ToChunkIndex(glm::ivec3 globalBlockpos);
    }
}