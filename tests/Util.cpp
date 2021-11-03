#include <doctest/doctest.h>

#include <util/Buffer.h>
#include <util/CircularVector.h>

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
        hey.Write("mate");

        CHECK(hey.GetSize() == 9);
        CHECK(hey.GetCapacity() == 132);
        str = (char*)hey.GetData();
        CHECK(str == "Hey mate");
    }
    SUBCASE("Reader")
    {
        Util::Buffer a;
        a.Write("Hello world\0");
        auto reader = a.GetReader();

        auto subBuffer = reader.Read(5);
        subBuffer.Write('\0');
        std::string hello((char*)subBuffer.GetData());
        CHECK("Hello" == hello);
        
        reader.Skip(1);
        auto world = reader.Read<std::string>();
        CHECK("world" == world);
        CHECK(reader.IsOver() == true);
    }
}


TEST_CASE("Queue")
{
    Util::Ring<int> q{5};

    SUBCASE("Push/Pop/GetSize")
    {
        auto size = q.GetSize();
        CHECK(size == 0);

        q.PushBack(1);
        size = q.GetSize();
        auto v = q.Front();

        CHECK(v == 1);
        CHECK(q.GetSize() == size);

        v = q.PopFront();
        CHECK(v == 1);
        CHECK(q.GetSize() == 0);
    }
    SUBCASE("Check FIFO")
    {
        for(auto i = 0; i < 5; i++)
            q.PushBack(i);

        for(auto i = 0; q.GetSize() > 0; i++)
            CHECK(q.PopFront() == i);
    }
    SUBCASE("Check removal of first member after max capacity")
    {
        for(auto i = 0; i < 6; i++)
            q.PushBack(i);

        CHECK(q.GetSize() == 5);
        for(auto i = 1; q.GetSize() > 0; i++)
            CHECK(q.PopFront() == i);
    }
    SUBCASE("Checking negative indices")
    {
        for(auto i = 0; i < 3; i++)
            q.PushBack(i);

        CHECK(q.At(0) == 0);
        CHECK(q.At(-1) == 2);
        CHECK(q.At(-2) == 1);
        CHECK(q.At(-3) == 0);
    }
}