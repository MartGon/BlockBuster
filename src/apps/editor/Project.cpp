#include <Project.h>

#include <debug/Debug.h>

#include <fstream>
#include <iostream>

BlockBuster::Editor::Project::Project()
{    

}

void BlockBuster::Editor::Project::Init()
{
    blockScale = 2.0f;
    colors = {glm::vec4{0.0f, 0.0f, 0.0f, 1.0f}, glm::vec4{1.0f, 0.0f, 0.0f, 1.0f}, glm::vec4{1.0f}};

    map.AddBlock(glm::ivec3{0}, Game::Block{ 
        Game::BLOCK, Game::ROT_0, Game::ROT_0, Game::Display{Game::DisplayType::COLOR, 2}
    });
}

template<typename T>
static void WriteToFile(std::fstream& file, T val)
{
    file.write(reinterpret_cast<char*>(&val), sizeof(T));
}

template<int S, typename T>
static void WriteToFile(std::fstream& file, T* val)
{
    file.write(reinterpret_cast<char*>(val), sizeof(T)*S);
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

static const int magicNumber = 0xB010F0;

void BlockBuster::Editor::WriteProjectToFile(BlockBuster::Editor::Project& p, std::filesystem::path filepath)
{
    std::fstream file{filepath, file.binary | file.out};
    if(!file.is_open())
    {
        std::cout << "Could not open file " << filepath << '\n';
        return;
    }

    WriteToFile(file, magicNumber);

    // Write map
    auto& map = p.map;
    auto chunkIndices = p.map.GetChunkIndices();

    WriteToFile(file, chunkIndices.size());
    for(auto chunkIndex : chunkIndices)
    {   
        auto& chunk = map.GetChunk(chunkIndex);
        auto i8Index = glm::lowp_i8vec3{chunkIndex};
        WriteToFile(file, i8Index);

        auto blockCount = chunk.GetBlockCount();
        WriteToFile(file, blockCount);

        auto chunkIt = chunk.CreateBlockIterator();
        for(auto b = chunkIt.GetNextBlock(); !chunkIt.IsOver(); b = chunkIt.GetNextBlock())
        {
            // Write pos - block pair
            glm::lowp_i8vec3 pos = b.first;
            WriteToFile(file, pos);

            auto block = b.second;
            WriteToFile(file, b.second->type);
            if(block->type == Game::BlockType::SLOPE)
                WriteToFile(file, block->rot);
            WriteToFile(file, block->display);
        }
    }

    // Write blockscale
    WriteToFile(file, p.blockScale);

    // Write textures
    WriteToFile(file, p.textureFolder.string());

    auto texCount = p.palette.GetCount();
    WriteToFile(file, texCount);
    for(auto i = 0; i < texCount; i++)
    {
        auto texturePath =  p.palette.GetMember(i).filepath;
        auto textureName = texturePath.filename().string();
        WriteToFile(file, textureName);
    }

    // Colors table
    WriteToFile(file, p.colors.size());
    for(auto i = 0; i < p.colors.size(); i++)
    {
        WriteToFile(file, p.colors[i]);
    }

    // Camera Pos/Rot
    WriteToFile(file, p.cameraPos);
    WriteToFile(file, p.cameraRot);

    // Cursor scale/pos
    WriteToFile(file, p.cursorPos);
    WriteToFile(file, p.cursorScale);
}

BlockBuster::Editor::Project BlockBuster::Editor::ReadProjectFromFile(std::filesystem::path filepath)
{
    Project p;

    std::fstream file{filepath, file.binary | file.in};
    if(!file.is_open())
    {
        std::cout << "Could not open file " << filepath << '\n';
        p.isOk = false;
        return p;
    }

    auto magic = ReadFromFile<int>(file);
    if(magic != magicNumber)
    {
        std::cout << "Wrong file format for file " << filepath << "\n";
        p.isOk = false;
        return p;
    }

    // Load map
    auto chunkIndicesCount = ReadFromFile<std::size_t>(file);
    for(auto i = 0; i < chunkIndicesCount; i++)
    {
        auto chunkIndex = ReadFromFile<glm::lowp_i8vec3>(file);
        auto blockCount = ReadFromFile<unsigned int>(file);
        for(auto b = 0; b < blockCount; b++)
        {
            auto chunkBlockPos = ReadFromFile<glm::lowp_i8vec3>(file);
            auto globalPos = Game::Map::ToGlobalPos(chunkIndex, chunkBlockPos);
            
            Game::Block block;
            block.type = ReadFromFile<Game::BlockType>(file);
            if(block.type == Game::BlockType::SLOPE)
                block.rot = ReadFromFile<Game::BlockRot>(file);
            block.display = ReadFromFile<Game::Display>(file);

            p.map.AddBlock(globalPos, block);
        }
    }

    // Load blockScale
    p.blockScale = ReadFromFile<float>(file);

    // Load textures
    p.textureFolder = ReadFromFile<std::string>(file);

    auto textureSize = ReadFromFile<std::size_t>(file);
    for(auto i = 0; i < textureSize; i++)
    {
        auto filename = ReadFromFile<std::string>(file);
        auto texturePath = p.textureFolder / filename;
        auto id = p.palette.AddTexture(p.textureFolder, filename);
    }

    // Color Table
    p.colors.clear();
    auto colorsSize = ReadFromFile<std::size_t>(file);
    p.colors.reserve(colorsSize);
    for(auto i = 0; i < colorsSize; i++)
    {
        auto color = ReadFromFile<glm::vec4>(file);
        p.colors.push_back(color);
    }

    // Camera Pos/Rot
    p.cameraPos = ReadFromFile<glm::vec3>(file);
    p.cameraRot = ReadFromFile<glm::vec2>(file);

    // Cursor Pos/Scale
    p.cursorPos = ReadFromFile<glm::ivec3>(file);
    p.cursorScale = ReadFromFile<glm::ivec3>(file);

    return p;
}