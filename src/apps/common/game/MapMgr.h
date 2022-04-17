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

        bool HasMap(const std::string& map);
        std::filesystem::path GetMapFolder(const std::string& mapName);
        std::filesystem::path GetMapFile(const std::string& mapName);
        std::vector<std::filesystem::path> GetLocalMaps();

        void WriteMapVersion(const std::string& map, const std::string& version);
        std::string ReadMapVersion(const std::string& map);

    private:
        std::filesystem::path mapsFolder;
    };
}