#pragma once

#include <Player.h>
#include <Map.h>

#include <util/Timer.h>
#include <util/Buffer.h>

#include <mglogger/Logger.h>

#include <unordered_map>
#include <vector>
#include <optional>
#include <functional>

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

        virtual void OnPlayerDeath(Entity::ID killer, Entity::ID victim) {}; // Change score, check for game over, etc.

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

        void OnPlayerDeath(Entity::ID killer, Entity::ID victim) override;
        bool IsGameOver() override;
        int MAX_KILLS = 30;
    };

    class Match
    {
    public:
        enum StateType
        {
            WAITING_FOR_PLAYERS,
            ON_GOING,
            ENDING,
            ENDED
        };

        struct State
        {
            GameMode::Type gameMode;
            StateType state;
            Util::Time::Seconds timeLeft;
        };

        inline GameMode* GetGameMode()
        {
            return gameMode.get();
        }

        inline StateType GetState()
        {
            return state;
        }

        inline bool IsOver()
        {
            return state == ENDED;
        }

        inline bool IsOnGoing()
        {
            return state == ON_GOING;
        }

        inline Util::Time::Seconds GetTimeLeft()
        {
            return timer.GetTimeLeft();
        }

        inline State ExtractState()
        {
            State state;
            state.state = this->state;
            state.timeLeft = timer.GetTimeLeft();

            return state;
        }

        inline void ApplyState(State state)
        {
            this->state = state.state;
            this->timer.SetDuration(state.timeLeft);
        }

        inline void SetOnEnterState(std::function<void(StateType type)> onEnterState)
        {
            this->onEnterState = onEnterState;
        }

        void Start(GameMode::Type type);
        void Update(Log::Logger* logger, Util::Time::Seconds deltaTime);

    private:

        void EnterState(StateType type);

        std::unique_ptr<GameMode> gameMode;
        StateType state = WAITING_FOR_PLAYERS;

        const Util::Time::Seconds gameTime{60.0f * 12}; // 12 min
        const Util::Time::Seconds waitTime{15.0f};
        Util::Timer timer{waitTime};

        std::function<void(StateType type)> onEnterState;
    };
}