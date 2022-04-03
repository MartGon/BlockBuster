#pragma once

#include <GameState/GameState.h>
#include <GameState/MainMenu/MenuState.h>
#include <GameState/MainMenu/MainMenuFwd.h>

#include <gl/VertexArray.h>
#include <gl/Texture.h>
#include <gl/Shader.h>

#include <gui/PopUp.h>
#include <util/Ring.h>
#include <http/AsyncClient.h>

#include <nlohmann/json.hpp>

#include <game/MapMgr.h>

#include <thread>
#include <mutex>
#include <atomic>
#include <functional>

namespace BlockBuster
{
    class Client;


    class MainMenu : public GameState
    {
    friend class Client;
    friend class InGame;
    friend class MenuState::Login;
    friend class MenuState::ServerBrowser;
    friend class MenuState::CreateGame;
    friend class MenuState::Lobby;
    friend class MenuState::UploadMap;

    public:
        MainMenu(Client* client);
        ~MainMenu();
        
        void Start() override;
        void Shutdown() override;
        void Update() override;

    private:

        //#### Function Members ####\\
        // REST service
        void Login(std::string userName);
        void ListGames();
        void JoinGame(std::string gameId);
        void CreateGame(std::string name, std::string map, std::string mode, uint8_t max_players);
        void GetAvailableMaps();
        void LeaveGame();
        void ToggleReady();
        void SendChatMsg(std::string msg);
        void UpdateGame(bool forced = false);
        void StartGame();
        void DownloadMap(std::string mapName);
        void GetMapPicture(std::string mapName);
        void UploadMap(std::string mapName, std::string password);

        // Inputs
        void HandleSDLEvents();

        // Rendering
        void Render();
        void DrawGUI();

        // GUI
        void SetState(std::unique_ptr<MenuState::Base> menuState_);

        // Handy 
        void LaunchGame();
        MapMgr& GetMapMgr();

        //#### Data Members ####\\
        // Rest Service
        HTTP::AsyncClient httpClient;

        // GUI
        GL::VertexArray guiVao;
        struct MapPic
        {
            GL::Texture texture;
            ImGui::Impl::Texture imGuiImpl;
        };
        std::unordered_map<std::string, MapPic> mapPics;
        GUI::BasicPopUp popUp;
        std::unique_ptr<MenuState::Base> menuState_;

        // GUI - Windows
        // Login
        std::string userId;
        std::string user;

        // Server Browser
        std::vector<GameInfo> gamesList;

        // Create Game
        std::vector<MapInfo> availableMaps;

        // Lobby        
        std::optional<GameDetails> currentGame;
        MenuState::Lobby* lobby = nullptr;
        bool enteringGame = false;
    };
}