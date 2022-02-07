#pragma once

#include <util/Buffer.h>

#include <fstream>

namespace Util::File
{
    void WriteToFile(std::fstream& file, void* data, uint32_t size);
    void WriteToFile(std::fstream& file, std::string str);

    template<typename T>
    void WriteToFile(std::fstream& file, T val)
    {
        file.write(reinterpret_cast<char*>(&val), sizeof(T));
    }

    template <typename T>
    T ReadFromFile(std::fstream& file)
    {
        T val;
        file.read(reinterpret_cast<char*>(&val), sizeof(T));
        return val;
    }

    Util::Buffer ReadFromFile(std::fstream& file, uint32_t size);
}