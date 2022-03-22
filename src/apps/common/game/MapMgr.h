#pragma once

#include <optional>
#include <filesystem>

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
        std::optional<std::filesystem::path> GetMapPath(std::string mapName);

    private:
        std::filesystem::path mapsFolder;
    };
}