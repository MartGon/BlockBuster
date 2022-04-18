#include <MapMgr.h>

#include <fstream>

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

void MapMgr::WriteMapVersion(const std::string& map, const std::string& version)
{
    auto mapFolder = GetMapFolder(map);
    auto versionFilePath = mapFolder / "version.txt";
    std::fstream versionFile{versionFilePath, versionFile.out};

    versionFile.write(version.c_str(), version.size());
    versionFile.close();
}

std::string MapMgr::ReadMapVersion(const std::string& map)
{
    auto mapFolder = GetMapFolder(map);
    auto versionFilePath = mapFolder / "version.txt";
    std::fstream versionFile{versionFilePath, versionFile.in};

    std::string version;
    if(!versionFile.bad())
        std::getline(versionFile, version);

    return version;
}