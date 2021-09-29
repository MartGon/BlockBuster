#include <Project.h>

#include <debug/Debug.h>

#include <fstream>
#include <iostream>

BlockBuster::Editor::Project::Project()
{    

}

void BlockBuster::Editor::Project::Init()
{
    map.SetBlockScale(2.0f);
    const auto black = glm::u8vec4{0, 0, 0, 255};
    const auto red = glm::u8vec4{255, 0, 0, 255};
    const auto white = glm::u8vec4{255};
    map.cPalette.AddColor(white);
    map.cPalette.AddColor(black);
    map.cPalette.AddColor(red);

    map.SetBlock(glm::ivec3{0}, Game::Block{ 
        Game::BLOCK, Game::ROT_0, Game::ROT_0, Game::Display{Game::DisplayType::COLOR, 2}
    });
}

static void WriteToFile(std::fstream& file, void* data, uint32_t size)
{
    file.write(reinterpret_cast<char*>(data), size);
}

template<typename T>
static void WriteToFile(std::fstream& file, T val)
{
    file.write(reinterpret_cast<char*>(&val), sizeof(T));
}

static void WriteToFile(std::fstream& file, std::string str)
{
    for(auto c : str)
        WriteToFile(file, c);
    WriteToFile(file, '\0');
}

template <typename T>
static T ReadFromFile(std::fstream& file)
{
    T val;
    file.read(reinterpret_cast<char*>(&val), sizeof(T));
    return val;
}

template <>
std::string ReadFromFile(std::fstream& file)
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

static Util::Buffer ReadFromFile(std::fstream& file, uint32_t size)
{
    Util::Buffer buffer{size};
    file.read(reinterpret_cast<char*>(buffer.GetData()), size);
    return std::move(buffer);
}

static const int magicNumber = 0xB010F0;

void BlockBuster::Editor::WriteProjectToFile(BlockBuster::Editor::Project& p, std::filesystem::path filepath, Log::Logger* logger)
{
    std::fstream file{filepath, file.binary | file.out};
    if(!file.is_open())
    {
        if(logger)
            logger->LogError("Could not open file " + filepath.string());
        return;
    }

    WriteToFile(file, magicNumber);

    // Write map
    auto buffer = p.map.ToBuffer();
    auto size = buffer.GetSize();
    WriteToFile(file, size);
    WriteToFile(file, buffer.GetData(), size);

    // Camera Pos/Rot
    WriteToFile(file, p.cameraPos);
    WriteToFile(file, p.cameraRot);

    // Cursor scale/pos
    WriteToFile(file, p.cursorPos);
    WriteToFile(file, p.cursorScale);
}

BlockBuster::Editor::Project BlockBuster::Editor::ReadProjectFromFile(std::filesystem::path filepath, Log::Logger* logger)
{
    Project p;

    std::fstream file{filepath, file.binary | file.in};
    if(!file.is_open())
    {
        if(logger)
            logger->LogError("Could not open file " + filepath.string());
        p.isOk = false;
        return p;
    }

    auto magic = ReadFromFile<int>(file);
    if(magic != magicNumber)
    {
        if(logger)
            logger->LogError("Wrong format for file " + filepath.string());
        p.isOk = false;
        return p;
    }

    // Load map
    auto bufferSize = ReadFromFile<uint32_t>(file);
    Util::Buffer buffer = ReadFromFile(file, bufferSize);
    p.map = App::Client::Map::FromBuffer(buffer.GetReader(), logger);

    // Camera Pos/Rot
    p.cameraPos = ReadFromFile<glm::vec3>(file);
    p.cameraRot = ReadFromFile<glm::vec2>(file);

    // Cursor Pos/Scale
    p.cursorPos = ReadFromFile<glm::ivec3>(file);
    p.cursorScale = ReadFromFile<glm::ivec3>(file);

    return p;
}