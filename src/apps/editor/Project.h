#pragma once

#include <game/Map.h>
#include <rendering/TexturePalette.h>
#include <rendering/ColorPalette.h>

#include <client/Map.h>

#include <mglogger/MGLogger.h>

namespace BlockBuster::Editor
{
    struct TextureInfo
    {
        GLuint id;
        std::filesystem::path path;
    };

    class Project
    {
    public:
        Project();

        void Init();

        Game::Map::Map map;
        Rendering::TexturePalette tPalette{16};
        Rendering::ColorPalette cPalette{32};
        std::filesystem::path textureFolder;
        App::Client::Map cMap;
        
        float blockScale;

        glm::vec3 cameraPos;
        glm::vec2 cameraRot;

        glm::ivec3 cursorPos;
        glm::ivec3 cursorScale;

        // Control flag
        bool isOk = true;
    };

    void WriteProjectToFile(BlockBuster::Editor::Project& p, std::filesystem::path filepath, Log::Logger* logger = nullptr);
    Project ReadProjectFromFile(std::filesystem::path filepath, Log::Logger* logger = nullptr);
}