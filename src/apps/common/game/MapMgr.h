#pragma once

#include <optional>
#include <filesystem>
#include <vector>

namespace BlockBuster
{
    class MapMgr
    {
    public:

        inline void SetMapsFolder(std::filesystem::path mapsFolder)
        {
            this->mapsFolder = mapsFolder;
        }

        inline std::filesystem::path GetMapsFolder()
        {
            return this->mapsFolder;
        }

        bool HasMap(std::string map);
        std::filesystem::path GetMapPath(std::string mapName);
        std::vector<std::filesystem::path> GetLocalMaps();

    private:
        std::filesystem::path mapsFolder;
    };
}