#include <File.h>

using namespace Util;
using namespace Util::File;


void File::WriteToFile(std::fstream& file, void* data, uint32_t size)
{
    file.write(reinterpret_cast<char*>(data), size);
}


void File::WriteToFile(std::fstream& file, std::string str)
{
    for(auto c : str)
        WriteToFile(file, c);
    WriteToFile(file, '\0');
}

template <>
std::string File::ReadFromFile(std::fstream& file)
{
    std::string str;
    char c = ReadFromFile<char>(file);
    while(c != '\0' && !file.eof())
    {   
        str.push_back(c);
        c = ReadFromFile<char>(file);
    }

    return str;
}

Util::Buffer File::ReadFromFile(std::fstream& file, uint32_t size)
{
    return Util::Buffer::FromFile(file, size);
}