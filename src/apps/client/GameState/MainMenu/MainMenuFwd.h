#pragma once

#include <nlohmann/json.hpp>

namespace BlockBuster
{
    struct GameInfo{
        std::string id;
        std::string name;
        std::string map;
        std::string map_version;
        std::string mode;
        uint8_t players;
        uint8_t maxPlayers;
        uint16_t ping;
        std::vector<std::string> chat;

        std::optional<std::string> address;
        std::optional<uint16_t> serverPort;
        std::string state;

        static GameInfo FromJson(nlohmann::json game)
        {
            GameInfo gameInfo;
            gameInfo.id = game.at("id").get<std::string>();
            gameInfo.name = game.at("name").get<std::string>();
            gameInfo.map = game.at("map").get<std::string>();
            gameInfo.map_version = game.at("map_version").get<std::string>();
            gameInfo.mode = game.at("mode").get<std::string>();
            gameInfo.maxPlayers = game.at("max_players").get<uint8_t>();
            gameInfo.players = game.at("players").get<uint8_t>();
            gameInfo.ping = game.at("ping").get<uint16_t>();

            auto chatMsgs = game.at("chat").get<nlohmann::json::array_t>();
            for(auto msg : chatMsgs)
            {
                auto str = msg.get<std::string>();
                gameInfo.chat.push_back(str);
            }

            auto address = game.at("address");
            if(!address.is_null())
                gameInfo.address = address.get<std::string>();
            
            auto port = game.at("port");
            if(!port.is_null())
                gameInfo.serverPort = port.get<uint16_t>();

            gameInfo.state = game.at("state").get<std::string>();
            
            return gameInfo;
        }
    };

    struct MapInfo
    {
        std::string mapName;
        std::vector<std::string> supportedGamemodes;

        static MapInfo FromJson(nlohmann::json json)
        {
            MapInfo mapInfo;
            mapInfo.mapName = json.at("map_name").get<std::string>();
            auto gameModes = json.at("supported_gamemodes").get<nlohmann::json::array_t>();
            for(auto gameMode : gameModes)
            {
                auto mode = gameMode.get<std::string>();
                mapInfo.supportedGamemodes.push_back(mode);
            }

            return mapInfo;
        }
    };

    struct PlayerInfo
    {
        std::string playerName;
        bool isReady;
        bool isHost;

        static PlayerInfo FromJson(nlohmann::json json)
        {
            PlayerInfo playerInfo;
            playerInfo.playerName = json.at("name").get<std::string>();
            playerInfo.isReady = json.at("ready").get<bool>();
            playerInfo.isHost = json.at("host").get<bool>();
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
}