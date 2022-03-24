#include <MapMgr.h>

using namespace BlockBuster;

bool MapMgr::HasMap(const std::string& mapName)
{
    auto mapFolder = mapsFolder / mapName;

    return std::filesystem::is_directory(mapFolder);
}

std::filesystem::path MapMgr::GetMapFolder(const std::string& mapName)
{
    auto mapFolder = mapsFolder / mapName;

    return mapFolder;
}

std::filesystem::path MapMgr::GetMapFile(const std::string& mapName)
{
    return GetMapFolder(mapName) / (mapName + ".bbm");
}

std::vector<std::filesystem::path> MapMgr::GetLocalMaps()
{
    std::vector<std::filesystem::path> maps;

    for(const auto& entry : std::filesystem::directory_iterator(mapsFolder))
    {
        if(entry.is_directory())
        {
            maps.push_back(entry.path());
        }
    }

    return maps;
}