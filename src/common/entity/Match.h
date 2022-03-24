#pragma once

#include <Player.h>
#include <Map.h>

#include <unordered_map>
#include <vector>

namespace BlockBuster
{
    struct PlayerScore
    {
        Entity::ID playerId;
        std::string name;
        uint32_t score;
        uint32_t deaths;
    };
    using Scoreboard = std::unordered_map<Entity::ID, PlayerScore>;

    enum GameModeType
    {
        FREE_FOR_ALL,
        TEAM_DEATHMATCH,
        DOMINATION,
        CAPTURE_THE_FLAG,

        COUNT
    };

    class GameMode
    {
    public:
        // This would need map and player data
        virtual void Start() {}; // Spawn players
        virtual void Update() {}; // Check for domination points, flag carrying

        virtual void OnPlayerDeath(Entity::ID killer, Entity::ID victim) {}; // Change score, check for game over, etc.

        virtual bool IsGameOver() = 0;        

        static const std::string typeStrings[GameModeType::COUNT];
    private:
        Scoreboard scoreBoard;
    };
    std::unordered_map<GameModeType, bool> GetSupportedGameModes(Game::Map::Map& map);
    std::vector<std::string> GetSupportedGameModesAsString(Game::Map::Map& map);

    struct Match
    {
        enum State
        {
            WAITING_FOR_PLAYERS,
            ON_GOING,
            ENDED
        };

        std::unordered_map<Entity::ID, Entity::Player> players;
        ::Game::Map::Map map;
        GameMode* gameMode;
    };
}