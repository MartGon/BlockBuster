#pragma once

#include <game/Map.h>
#include <rendering/TexturePalette.h>

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
        Rendering::TexturePalette palette{16, 160};
        std::vector<glm::vec4> colors;
        float blockScale;

        glm::vec3 cameraPos;
        glm::vec2 cameraRot;

        std::filesystem::path textureFolder;

        glm::ivec3 cursorPos;
        glm::ivec3 cursorScale;

        // Control flag
        bool isOk = true;
    };

    void WriteProjectToFile(BlockBuster::Editor::Project& p, std::filesystem::path filepath);
    Project ReadProjectFromFile(std::filesystem::path filepath);
}