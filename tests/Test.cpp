#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <math/BBMath.h>
#include <blockbuster/Map.h>

#include <util/Serializable.h>

TEST_CASE("Math tests")
{
    CHECK(Math::OverflowSumInt(1, 3, 0, 3) == 0);
    CHECK(Math::OverflowSumInt(0, -1, 0, 3) == 3);
    CHECK(Math::OverflowSumInt(2, 1, -2, 2) == -2);
}

TEST_CASE("Map tests")
{
    SUBCASE("To ChunkPos")
    {
        auto chunkPos = Game::Map::ToChunkIndex(glm::vec3{17, 36, 27}, 1.0f);
        CHECK(chunkPos == glm::ivec3{1, 2, 2});

        chunkPos = Game::Map::ToChunkIndex(glm::vec3{0, 0, 0}, 1.0f);
        CHECK(chunkPos == glm::ivec3{0});

        chunkPos = Game::Map::ToChunkIndex(glm::vec3{15, 15, 15}, 1.0f);
        CHECK(chunkPos == glm::ivec3{1, 1, 1});

        chunkPos = Game::Map::ToChunkIndex(glm::vec3{-1, -1, -1}, 1.0f);
        CHECK(chunkPos == glm::ivec3{0});

        chunkPos = Game::Map::ToChunkIndex(glm::vec3{-17, -17, -17}, 1.0f);
        CHECK(chunkPos == glm::ivec3{-1});
    }
    SUBCASE("To Global Chunk Pos")
    {
        auto chunkIndex = glm::ivec3{1, 2, -1};
        auto globalPos = Game::Map::ToRealChunkPos(chunkIndex, 1.0f);
        CHECK(globalPos == glm::vec3{16, 32, -16});

        globalPos = Game::Map::ToRealChunkPos(chunkIndex, 2.0f);
        CHECK(globalPos == glm::vec3{32, 64, -32});
    }
    SUBCASE("To GlobalBlockPos")
    {
        auto chunkIndex = glm::ivec3{1, 2, -1};
        auto blockIndex = glm::ivec3{0, 0, 0};
        glm::vec3 globalPos = Game::Map::ToGlobalPos(chunkIndex, blockIndex);
        CHECK(globalPos == glm::vec3{16, 32, -16});

        blockIndex = glm::ivec3{0, -8, 0};
        globalPos = Game::Map::ToGlobalPos(chunkIndex, blockIndex);
        CHECK(globalPos == glm::vec3{16, 24, -16});
    }
}

TEST_CASE("Buffer tests")
{
    Util::Buffer buffer{128};
    
    SUBCASE("Check Resizing")
    {
        for(uint8_t i = 0; i < 128; i++)
            buffer.Write(i);

        CHECK(buffer.GetCapacity() == 128);
        CHECK(buffer.GetSize() == 128);

        for(uint8_t i = 0; i < 12; i++)
            buffer.Write(i);

        CHECK(buffer.GetCapacity() == 256);
        CHECK(buffer.GetSize() == 140);
    }
    SUBCASE("Check write and read")
    {
        const int value = 354;
        buffer.WriteAt(value, 0);
        auto read = buffer.ReadAt<int>(0);
        CHECK(value == read);
    }
    SUBCASE("Concat")
    {
        Util::Buffer a{3};
        Util::Buffer b{1};
        a.Write<char>('H');
        a.Write<char>('e');
        a.Write<char>('y');
        b.Write<char>('\0');

        a.Append(std::move(b));
        auto hey = std::move(a);
        CHECK(hey.GetSize() == 4);
        CHECK(hey.GetCapacity() == 4);

        std::string str((char*)hey.GetData());
        CHECK(str == "Hey");

        hey.WriteAt(' ', 3);
        hey.WriteStr("mate");

        CHECK(hey.GetSize() == 9);
        CHECK(hey.GetCapacity() == 132);
        str = (char*)hey.GetData();
        CHECK(str == "Hey mate");
    }
    SUBCASE("Reader")
    {
        Util::Buffer a;
        a.WriteStr("Hello world\0");
        auto reader = a.GetReader();

        auto subBuffer = reader.Read(5);
        subBuffer.Write('\0');
        std::string hello((char*)subBuffer.GetData());
        CHECK("Hello" == hello);
        
        reader.Skip(1);
        auto world = reader.ReadStr();
        CHECK("world" == world);
        CHECK(reader.IsOver() == true);
    }
}
