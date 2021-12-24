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
        void Shutdown() override;
        void Update() override;

    private:

        //#### Function Members ####\\
        // REST service
        void Login(std::string userName);
        void ListGames();
        void JoinGame(std::string gameId);
        void CreateGame(std::string name, std::string map, std::string mode, uint8_t max_players);
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
        struct GameInfo{
            std::string id;
            std::string name;
            std::string map;
            std::string mode;
            uint8_t players;
            uint8_t maxPlayers;
            uint16_t ping;

            static GameInfo FromJson(nlohmann::json game)
            {
                GameInfo gameInfo;
                gameInfo.id = game.at("id").get<std::string>();
                gameInfo.name = game.at("name").get<std::string>();
                gameInfo.map = game.at("map").get<std::string>();
                gameInfo.mode = game.at("mode").get<std::string>();
                gameInfo.maxPlayers = game.at("max_players").get<uint8_t>();
                gameInfo.players = game.at("players").get<uint8_t>();
                gameInfo.ping = game.at("ping").get<uint16_t>();
                return gameInfo;
            }
        };
        std::vector<GameInfo> gamesList;

        // Game
        struct PlayerInfo
        {
            std::string playerName;
            bool isReady;

            static PlayerInfo FromJson(nlohmann::json json)
            {
                PlayerInfo playerInfo;
                playerInfo.playerName = json.at("name").get<std::string>();
                playerInfo.isReady = json.at("ready").get<bool>();
                return playerInfo;
            }
        };
        struct GameDetails
        {
            GameInfo game;
            std::vector<PlayerInfo> playersInfo;

            static GameDetails FromJson(nlohmann::json json)
            {
                GameDetails gameDetails;
                gameDetails.game = GameInfo::FromJson(json.at("game_info"));
                
                auto players = json.at("players").get<nlohmann::json::array_t>();
                for(auto& player : players)
                {
                    auto playerInfo = PlayerInfo::FromJson(player);
                    gameDetails.playersInfo.push_back(playerInfo);
                }

                return gameDetails;
            }
        };
        std::optional<GameDetails> currentGame;
    };
}