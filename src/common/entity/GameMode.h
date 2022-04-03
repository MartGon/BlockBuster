#pragma once

#include <Map.h>
#include <Player.h>
#include <util/Buffer.h>
#include <util/Timer.h>

#include <Scoreboard.h>

#include <vector>

#include <mglogger/Logger.h>

namespace BlockBuster
{
    struct World
    {
        Game::Map::Map* map;
        std::unordered_map<Entity::ID, Entity::Player*> players;
        Log::Logger* logger;
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

        enum EventType
        {
            POINT_CAPTURED,
            FLAG_TAKEN,
        };

        struct PointCaptured
        {
            glm::ivec3 pos;
            Entity::ID capturedBy;
        };

        struct Event
        {
            EventType type;
            union
            {
                PointCaptured pointCaptured;
            };
        };

        GameMode(Type type, uint32_t maxScore = 100, 
            Util::Time::Seconds duration = Util::Time::Seconds{60.0f * 15.0f}, 
            Util::Time::Seconds respawnTime = Util::Time::Seconds{5.0f}) : 
            type{type}, maxScore{maxScore}, duration{duration}, respawnTime{respawnTime}
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
        virtual void Start(World world) {}; // Spawn players
        virtual void Update(World world, Util::Time::Seconds deltaTime) {}; // Check for domination points, flag carrying

        Entity::ID PlayerJoin(Entity::ID playerId, std::string name);
        void PlayerLeave(Entity::ID playerId);
        void PlayerDeath(Entity::ID killer, Entity::ID victim, Entity::ID killerTeamId);

        virtual bool IsGameOver();
        virtual Util::Time::Seconds GetDuration(){ return duration; };
        virtual Util::Time::Seconds GetRespawnTime() { return respawnTime; };

        std::vector<Event> PollEvents();

        static const std::string typeStrings[Type::COUNT];
        static const std::unordered_map<std::string, Type> stringTypes;
    protected:

        virtual Entity::ID OnPlayerJoin(Entity::ID playerId, std::string name) { return 0; };
        virtual void OnPlayerLeave(Entity::ID playerId) {};
        virtual void OnPlayerDeath(Entity::ID killer, Entity::ID victim, Entity::ID killerTeamId) {}; // Change score, check for game over, etc.
    
        void AddEvent(Event event);

        Type type;
        Scoreboard scoreBoard;
        Util::Time::Seconds duration;
        Util::Time::Seconds respawnTime;
        uint32_t maxScore;

        std::vector<Event> events;
    };
    std::unique_ptr<GameMode> CreateGameMode(GameMode::Type type);
    std::unordered_map<GameMode::Type, bool> GetSupportedGameModes(Game::Map::Map& map);
    std::vector<std::string> GetSupportedGameModesAsString(Game::Map::Map& map);

    class FreeForAll : public GameMode
    {
    public:
        FreeForAll() : GameMode(Type::FREE_FOR_ALL, 30, Util::Time::Seconds{60.0f * 10.0f})
        {

        }

    protected:
        Entity::ID OnPlayerJoin(Entity::ID id, std::string name) override;  
        void OnPlayerLeave(Entity::ID id) override;
        void OnPlayerDeath(Entity::ID killer, Entity::ID victim, Entity::ID killerTeamId) override;
    };

    class TeamGameMode : public GameMode
    {
    public:
        TeamGameMode(Type type, uint32_t maxScore = 100, 
            Util::Time::Seconds duration = Util::Time::Seconds{60.0f * 15.0f}, 
            Util::Time::Seconds respawnTime = Util::Time::Seconds{5.0f}) : 
            GameMode(type, maxScore, duration, respawnTime)
        {

        }

        enum TeamID : Entity::ID
        {
            BLUE_TEAM_ID,
            RED_TEAM_ID,
            NEUTRAL
        };

    protected:

        Entity::ID OnPlayerJoin(Entity::ID id, std::string name) override;
    };

    class TeamDeathMatch : public TeamGameMode
    {
    public:
        TeamDeathMatch() : TeamGameMode(Type::TEAM_DEATHMATCH, 100, Util::Time::Seconds{60.0f * 12.0f})
        {

        }

    protected:
        void OnPlayerDeath(Entity::ID killer, Entity::ID victim, Entity::ID killerTeamId) override;
    };

    class Domination : public TeamGameMode
    {
    public:
        static const Util::Time::Seconds timeToCapture;
        static const Util::Time::Seconds pointsTime;
        struct PointState
        {
            glm::ivec3 index;
            Entity::ID capturedBy;
            Util::Timer timeLeft;

            inline float GetCapturePercent()
            {
                return timeLeft.GetElapsedTime().count() / timeToCapture.count();
            }
        };

        Domination() : TeamGameMode(Type::DOMINATION, 200, Util::Time::Seconds{60.0f * 15.0f})
        {

        }

        void Start(World world);
        void Update(World world, Util::Time::Seconds deltaTime);

        bool IsPlayerInPointArea(World world, glm::ivec3 pos, Entity::Player* player);

        std::unordered_map<glm::ivec3, PointState> pointsState;
    protected:
        Util::Timer pointsTimer;
    };
}
