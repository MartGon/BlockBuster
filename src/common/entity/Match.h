#pragma once

#include <Player.h>
#include <Map.h>

#include <unordered_map>

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

    class GameMode
    {
    public:
        // This would need map and player data
        virtual void Start() {}; // Spawn players
        virtual void Update() {}; // Check for domination points, flag carrying

        virtual void OnPlayerDeath(Entity::ID killer, Entity::ID victim) {}; // Change score, check for game over, etc.

        virtual bool IsGameOver() = 0;        
    private:
        Scoreboard scoreBoard;
    };

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