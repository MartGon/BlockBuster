#pragma once

#include <game/Map.h>

#include <gl/Texture.h>

namespace BlockBuster::Editor
{
    class Project
    {
    public:
        Project();

        void Init();

        Game::Map::Map map;
        float blockScale;
        std::vector<GL::Texture> textures;
        std::vector<glm::vec4> colors;

        glm::vec3 cameraPos;
        glm::vec2 cameraRot;

        std::filesystem::path textureFolder;

        // Control flag
        bool isOk = true;
    };

    void WriteProjectToFile(BlockBuster::Editor::Project& p, std::filesystem::path filepath);
    Project ReadProjectFromFile(std::filesystem::path filepath);
}