#pragma once

#include <entity/Block.h>

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <util/Buffer.h>
#include <oktal/result.h>

#include <unordered_map>

#include <filesystem>
#include <memory>

namespace Game
{
    namespace Map
    {
        struct Respawn
        {
            glm::vec3 pos;
            float orientation;
            uint8_t teamId = 0;
        }; 

        class Map
        {
        public:
            class Chunk;

            Map() = default;

            Map(const Map& other) = delete;
            Map& operator=(const Map& other) = delete;

            Map(Map&&) = default;
            Map& operator=(Map&&) = default;

            // Gets a block by its global position
            Game::Block const* GetBlock(glm::ivec3 pos) const;
            // Sets a block by its global position
            void AddBlock(glm::ivec3 pos, Game::Block block);
            // Removes a block by its global position
            void RemoveBlock(glm::ivec3 pos);
            // Checks a block by its global position
            bool IsNullBlock(glm::ivec3 pos) const;

            std::vector<glm::ivec3> GetChunkIndices() const;
            // Gets a chunk by its chunk index
            Chunk& GetChunk(glm::ivec3 pos);
            bool HasChunk(glm::ivec3 index) const;
            void Clear();

            inline float GetBlockScale() const
            {
                return blockScale;
            }

            void SetBlockScale(float blockScale)
            {
                this->blockScale = blockScale;
            }

            unsigned int GetBlockCount() const;

            Util::Buffer ToBuffer();
            static Map FromBuffer(Util::Buffer::Reader& reader);

            enum class LoadMapError
            {
                FILE_NOT_FOUND,
                MAGIC_NUMBER_NOT_FOUND,
            };

            static const int magicNumber;
            static Result<std::shared_ptr<Map>, LoadMapError> LoadFromFile(std::filesystem::path file);

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

                // #### Index ##### \\
                // Gets a block by its internal index in the chunk [0, 15]
                Game::Block const* GetBlockByIndex(glm::ivec3 pos) const;
                // Sets a block by its internal index in the chunk [0, 15]
                void SetBlockByIndex(glm::ivec3 pos, Game::Block block);

                // #### Smart Index ##### \\
                // Get a block by smart index in the chunk [-16, 15]
                Game::Block const* GetBlockBySmartIndex(glm::ivec3 smartIndex);
                // Set block by smart index in the chunk [-16, 15];
                void SetBlockBySmartIndex(glm::ivec3 smartIndex, Game::Block block);

                /*
                    It has the following mapping. Similar to python
                    x: -16 -15 -14 -13 -12 -11 -10 -9 -8 -7 -6 -5 -4 -3 -2 -1 0 1  2 3  4  5   6  7  8 9 10 11 12 13 14 15
                    y: 0     1  2   3   4    5  6   7  8  9  10 11 12 13 14 15 0 1 2 3  4  5   6  7  8 9 10 11 12 13 14 15
                */
                static glm::ivec3 SmartIndexToIndex(glm::ivec3 smartIndex);

                // #### Pos ##### \\
                // Gets a block by its position within the chunk [-8, 7]
                Game::Block const* GetBlock(glm::ivec3 pos) const;
                // Sets a block by its position within the chunk [-8, 7]
                void SetBlock(glm::ivec3 pos, Game::Block block);

                /*
                    It has the following mapping. Zero is centered
                    x: -8 -7 -6 -5 -4 -3 -2 -1 0 1  2 3  4  5   6  7
                    y: 0  1  2  3   4  5  6  7 8 9  10 11 12 13 14 15
                */
                static glm::ivec3 PosToIndex(glm::ivec3 pos);
                
                // #### Smart Pos ##### \\
                // Gets a block by its smart position within the chunk [-16, 15]
                Game::Block const* GetBlockSmartPos(glm::ivec3 smartPos);

                // Sets a block by its smart position within the chunk [-16, 15]
                void SetBlockSmartPos(glm::ivec3 smartPos, Game::Block block);

                /*
                It has the following mapping. Useful when operating with neighbor chunks, as you can see
                    x: -16 -15 -14 -13 -12 -11 -10 -9 -8 -7 -6 -5 -4 -3 -2 -1 0 1  2 3  4  5   6  7  8 9 10 11 12 13 14 15
                    y: 8   9   10  11  12  13  14 15  0  1  2  3  4  5  6  7 8 9 10 11 12 13  14 15 0 1  2  3  4  5  6  7
                */
                static glm::ivec3 SmartPosToIndex(glm::ivec3 smartPos);

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
                int ToIndex(glm::ivec3 pos) const;
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
            float blockScale = 2.0f;
            std::unordered_map<glm::ivec3, Chunk> chunks_;
            std::vector<Respawn> respawns_;
        };

        // Position transforms

        // #### Blocks #### \\
        // GlobalPos: ivec3 coordinates of a block in the map
        // ChunkPos: ivec3 coordinates of a block relative to a chunk
        // RealPos: vec3 position of a block/chunk taking block scale into account

        // Returns real block pos from global block pos
        glm::vec3 ToRealPos(glm::ivec3 globalPos, float blockScale = 1.0f);

        // Returns real block pos from chunk index and block index [-8, 7] within that chunk.
        glm::vec3 ToRealPos(glm::ivec3 chunkIndex, glm::ivec3 blockIndex, float blockScale = 1.0f);

        // Returns the block corresponding to a real pos;
        glm::ivec3 ToGlobalPos(glm::vec3 realPos, float blockScale = 1.0f);

        // Returns global block pos from chunk index and block index [-8, 7] within that chunk. Ignores scale
        glm::ivec3 ToGlobalPos(glm::ivec3 chunkIndex, glm::ivec3 blockIndex);

        // Global block pos to block position within a chunk
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