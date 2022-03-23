#include <Project.h>

#include <debug/Debug.h>
#include <game/ServiceLocator.h>

#include <File.h>

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

void BlockBuster::Editor::WriteProjectToFile(BlockBuster::Editor::Project& p, std::filesystem::path folder, std::string fileName)
{
    using namespace Util::File;

    auto filepath = folder / fileName;
    std::fstream file{filepath, file.binary | file.out};
    if(!file.is_open())
    {
        if(auto logger = App::ServiceLocator::GetLogger())
            logger->LogError("Could not open file " + filepath.string());
        return;
    }

    WriteToFile(file, Game::Map::Map::magicNumber);

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

BlockBuster::Editor::Project BlockBuster::Editor::ReadProjectFromFile(std::filesystem::path folder, std::string fileName)
{
    using namespace Util::File;
    Project p;

    auto filepath = folder / fileName;
    if(!std::filesystem::is_regular_file(filepath))
    {
        if(auto logger = App::ServiceLocator::GetLogger())
            logger->LogError("Invalid path " + filepath.string() + ". It's not a file");
        p.isOk = false;
        return p;
    }

    std::fstream file{filepath, file.binary | file.in};
    if(!file.is_open())
    {
        if(auto logger = App::ServiceLocator::GetLogger())
            logger->LogError("Could not open file " + filepath.string());
        p.isOk = false;
        return p;
    }

    auto magic = ReadFromFile<int>(file);
    if(magic != Game::Map::Map::magicNumber)
    {
        if(auto logger = App::ServiceLocator::GetLogger())
            logger->LogError("Wrong format for file " + filepath.string());
        p.isOk = false;
        return p;
    }

    // Load map
    auto bufferSize = ReadFromFile<uint32_t>(file);
    Util::Buffer buffer = ReadFromFile(file, bufferSize);
    p.map = App::Client::Map::FromBuffer(buffer.GetReader(), folder);

    // Camera Pos/Rot
    p.cameraPos = ReadFromFile<glm::vec3>(file);
    p.cameraRot = ReadFromFile<glm::vec2>(file);

    // Cursor Pos/Scale
    p.cursorPos = ReadFromFile<glm::ivec3>(file);
    p.cursorScale = ReadFromFile<glm::ivec3>(file);

    return p;
}