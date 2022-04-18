#pragma once

#include <app/App.h>

#include <gl/Shader.h>
#include <gl/VertexArray.h>
#include <gl/Texture.h>

#include <rendering/Camera.h>
#include <rendering/Mesh.h>
#include <rendering/Primitive.h>
#include <rendering/Rendering.h>
#include <rendering/Skybox.h>

#include <entity/PlayerController.h>
#include <game/Map.h>
#include <game/CameraController.h>
#include <game/ChunkMeshMgr.h>

#include <util/BBTime.h>
#include <util/Ring.h>

#include <entity/Player.h>

#include <networking/enetw/ENetW.h>
#include <networking/Snapshot.h>

#include <game/MapMgr.h>

#include <GameState/GameState.h>
#include <GameState/InGame/InGame.h>
#include <GameState/MainMenu/MainMenu.h>

namespace BlockBuster
{
    class Client : public App::AppI
    {
    friend class GameState;
    friend class MainMenu;
    friend class InGame;
    friend class InGameGUI;

    public:
        Client(::App::Configuration config);

        void Start() override;
        void Shutdown() override;
        void Update() override;
        bool Quit() override;
        
    private:

        void LaunchGame(std::string address, uint16_t port, std::string map, std::string playerUuid, std::string playerName);
        void GoBackToMainMenu(bool onGoing);

        void ApplyVideoOptions(App::Configuration::WindowConfig& winConfig) override;

        std::unique_ptr<MainMenu> menu = nullptr;
        std::unique_ptr<InGame> inGame = nullptr;
        GameState* state = nullptr;

        // Map Mgr
        MapMgr mapMgr;

        // Skybox
        GL::Shader skyboxShader;
        Rendering::Skybox skybox;

        // App
        std::filesystem::path texturesDir;
        std::filesystem::path resourcesDir;
        bool quit = false;
    };
}