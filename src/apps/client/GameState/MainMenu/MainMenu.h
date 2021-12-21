#pragma once

#include <GameState/GameState.h>
#include <GameState/MainMenu/MenuState.h>

#include <gl/VertexArray.h>
#include <gui/PopUp.h>
#include <util/Ring.h>
#include <http/AsyncClient.h>

#include <nlohmann/json.hpp>

#include <thread>
#include <mutex>
#include <atomic>
#include <functional>

namespace BlockBuster
{
    class Client;

    class MainMenu : public GameState
    {
    friend class MenuState::Login;
    friend class MenuState::ServerBrowser;
    friend class MenuState::CreateGame;
    friend class MenuState::Lobby;

    public:
        MainMenu(Client* client);
        ~MainMenu();
        
        void Start() override;
        void Update() override;

    private:

        //#### Function Members ####\\
        // REST service
        void Login(std::string userName);
        void ListGames();
        void JoinGame(std::string gameId);
        void CreateGame(std::string name);
        void LeaveGame();

        // Inputs
        void HandleSDLEvents();

        // Rendering
        void Render();
        void DrawGUI();

        // GUI
        void SetState(std::unique_ptr<MenuState::Base> menuState_);

        //#### Data Members ####\\
        // Rest Service
        HTTP::AsyncClient httpClient;

        // GUI
        GL::VertexArray guiVao;
        GUI::PopUp popUp;
        std::unique_ptr<MenuState::Base> menuState_;

        // GUI - Windows
        // Login
        std::string userId;
        std::string user;

        // Server Browser
        struct Game{
            std::string id;
            std::string name;
            std::string map;
            std::string mode;
            uint8_t players;
            uint8_t maxPlayers;
            uint16_t ping;
        };
        std::vector<nlohmann::json> gamesList;

        // Game
        struct PlayerInfo
        {
            std::string playerName;
        };
        struct GameDetails
        {
            Game game;
            std::vector<PlayerInfo> playersInfo;
        };
        std::optional<nlohmann::json> currentGame;
    };
}