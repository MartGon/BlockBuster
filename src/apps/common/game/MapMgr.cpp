#include <MapMgr.h>

using namespace BlockBuster;

bool MapMgr::HasMap(std::string mapName)
{
    auto mapFolder = mapsFolder / mapName;

    return std::filesystem::is_directory(mapFolder);
}