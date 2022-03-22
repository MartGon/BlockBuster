#include <MapMgr.h>

using namespace BlockBuster;

bool MapMgr::HasMap(std::string mapName)
{
    auto mapFolder = mapsFolder / mapName;

    return std::filesystem::is_directory(mapFolder);
}

std::filesystem::path MapMgr::GetMapPath(std::string mapName)
{
    auto mapFolder = mapsFolder / mapName;

    return mapFolder;
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