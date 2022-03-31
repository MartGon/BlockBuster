#pragma once

#include <Map.h>
#include <Player.h>
#include <util/Buffer.h>

#include <vector>

namespace BlockBuster
{
    struct PlayerScore
    {
        Entity::ID playerId;
        std::string name;
        uint32_t kills;
        uint32_t score;
        uint32_t deaths;

        Util::Buffer ToBuffer();
        static PlayerScore FromBuffer(Util::Buffer::Reader& reader);
    };

    struct TeamScore
    {
        Entity::ID teamId;
        uint32_t score;
    };

    class Scoreboard
    {
    public:

        inline bool HasChanged() const
        {
            return hasChanged;
        }

        inline void CommitChanges()
        {
            hasChanged = false;
        }

        void AddPlayer(Entity::ID playerId, std::string name);
        void SetPlayerScore(PlayerScore ps);
        void RemovePlayer(Entity::ID playerId);
        std::vector<Entity::ID> GetPlayerIDs();
        std::optional<PlayerScore> GetPlayerScore(Entity::ID playerId);

        void AddTeam(Entity::ID teamId);
        void SetTeamScore(TeamScore teamScore);
        void RemoveTeam(Entity::ID teamId);
        std::vector<Entity::ID> GetTeamIDs();
        std::optional<TeamScore> GetTeamScore(Entity::ID teamId);

        Util::Buffer ToBuffer();
        static Scoreboard FromBuffer(Util::Buffer::Reader& reader);

    private:
        std::unordered_map<Entity::ID, PlayerScore> playersScore;
        std::unordered_map<Entity::ID, TeamScore> teamsScore;
        bool hasChanged = false;
    };

    class GameMode
    {
    public:
        enum Type
        {
            NULL_MODE,
            FREE_FOR_ALL,
            TEAM_DEATHMATCH,
            DOMINATION,
            CAPTURE_THE_FLAG,

            COUNT
        };

        GameMode(Type type) : type{type}
        {

        }
        virtual ~GameMode() {}

        inline Type GetType()
        {
            return type;
        }

        inline Scoreboard& GetScoreboard()
        {
            return this->scoreBoard;
        }

        inline void SetScoreboard(Scoreboard scoreboard)
        {
            this->scoreBoard = scoreboard;
        }

        // This would need map and player data
        virtual void Start() {}; // Spawn players
        virtual void Update() {}; // Check for domination points, flag carrying

        virtual void OnPlayerDeath(Entity::ID killer, Entity::ID victim, Entity::ID killerTeamId) {}; // Change score, check for game over, etc.

        virtual bool IsGameOver() = 0;

        static const std::string typeStrings[Type::COUNT];
        static const std::unordered_map<std::string, Type> stringTypes;
    protected:
        Type type;
        Scoreboard scoreBoard;
    };
    std::unique_ptr<GameMode> CreateGameMode(GameMode::Type type);
    std::unordered_map<GameMode::Type, bool> GetSupportedGameModes(Game::Map::Map& map);
    std::vector<std::string> GetSupportedGameModesAsString(Game::Map::Map& map);

    class FreeForAll : public GameMode
    {
    public:
        FreeForAll() : GameMode(Type::FREE_FOR_ALL)
        {

        }

        void OnPlayerDeath(Entity::ID killer, Entity::ID victim, Entity::ID killerTeamId) override;
        bool IsGameOver() override;
        int MAX_KILLS = 30;
    };

    class TeamDeathMatch : public GameMode
    {
    public:
        TeamDeathMatch() : GameMode(Type::TEAM_DEATHMATCH)
        {

        }

        void OnPlayerDeath(Entity::ID killer, Entity::ID victim, Entity::ID killerTeamId) override;
        bool IsGameOver() override;
        int MAX_KILLS = 100;
    };
}
