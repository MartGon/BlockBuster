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
        Entity::ID teamId;
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

        void AddPlayer(Entity::ID playerId, Entity::ID teamID, std::string name);
        void SetPlayerScore(PlayerScore ps);
        void RemovePlayer(Entity::ID playerId);
        std::vector<Entity::ID> GetPlayerIDs();
        std::vector<PlayerScore> GetPlayerScores();
        std::optional<PlayerScore> GetPlayerScore(Entity::ID playerId);

        void SetTeamScore(TeamScore teamScore);
        std::vector<Entity::ID> GetTeamIDs();
        std::optional<TeamScore> GetTeamScore(Entity::ID teamId);
        std::vector<TeamScore> GetTeamScores();
        
        std::optional<TeamScore> GetWinner();
        std::vector<PlayerScore> GetTeamPlayers(Entity::ID teamId);

        Util::Buffer ToBuffer();
        static Scoreboard FromBuffer(Util::Buffer::Reader& reader);

    private:
        void RemoveTeam(Entity::ID teamId);

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

        Entity::ID PlayerJoin(Entity::ID playerId, std::string name);
        void PlayerLeave(Entity::ID playerId);
        void PlayerDeath(Entity::ID killer, Entity::ID victim, Entity::ID killerTeamId);

        virtual bool IsGameOver() = 0;
        virtual Util::Time::Seconds GetDuration(){ return Util::Time::Seconds{60.0f * 15.0f}; };

        static const std::string typeStrings[Type::COUNT];
        static const std::unordered_map<std::string, Type> stringTypes;
    protected:

        virtual Entity::ID OnPlayerJoin(Entity::ID playerId, std::string name) { return 0; };
        virtual void OnPlayerLeave(Entity::ID playerId) {};
        virtual void OnPlayerDeath(Entity::ID killer, Entity::ID victim, Entity::ID killerTeamId) {}; // Change score, check for game over, etc.
    
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

        bool IsGameOver() override;
        Util::Time::Seconds GetDuration() override {return duration;};

    protected:
        Entity::ID OnPlayerJoin(Entity::ID id, std::string name) override;  
        void OnPlayerLeave(Entity::ID id) override;
        void OnPlayerDeath(Entity::ID killer, Entity::ID victim, Entity::ID killerTeamId) override;

    private:
        int MAX_KILLS = 30;
        const Util::Time::Seconds duration{60.0f * 10.0f};
    };

    class TeamGameMode : public GameMode
    {
    public:
        TeamGameMode(Type type) : GameMode(type)
        {

        }

        virtual bool IsGameOver() override = 0;

        enum TeamID : Entity::ID
        {
            BLUE_TEAM_ID,
            RED_TEAM_ID,
        };

    protected:

        Entity::ID OnPlayerJoin(Entity::ID id, std::string name) override;
    };

    class TeamDeathMatch : public TeamGameMode
    {
    public:
        TeamDeathMatch() : TeamGameMode(Type::TEAM_DEATHMATCH)
        {

        }

        bool IsGameOver() override;
        Util::Time::Seconds GetDuration() override {return duration;};

    protected:
        void OnPlayerDeath(Entity::ID killer, Entity::ID victim, Entity::ID killerTeamId) override;

    private:
        int MAX_KILLS = 100;
        const Util::Time::Seconds duration{60.0f * 12.0f};
    };
}
