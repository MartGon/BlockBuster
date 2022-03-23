#pragma once

#include <entity/Map.h>
#include <game/TexturePalette.h>
#include <game/ColorPalette.h>

#include <game/Map.h>

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

        App::Client::Map map;

        glm::vec3 cameraPos;
        glm::vec2 cameraRot;

        glm::ivec3 cursorPos;
        glm::ivec3 cursorScale;

        // Control flag
        bool isOk = true;
    };

    void WriteProjectToFile(BlockBuster::Editor::Project& p, std::filesystem::path folder, std::string fileName);
    Project ReadProjectFromFile(std::filesystem::path folder, std::string fileName);
}