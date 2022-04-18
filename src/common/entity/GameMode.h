#pragma once

#include <Map.h>
#include <Player.h>
#include <util/Buffer.h>
#include <util/Timer.h>
#include <Event.h>

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

    enum class MsgType
    {
        BROADCAST,
        PLAYER_TARGET
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

        std::vector<Event> PollEventMsgs(MsgType msgType = MsgType::BROADCAST, Entity::ID target = 0);
        void WipeEvents(); // Client - only

        static const std::string typeStrings[Type::COUNT];
        static const std::unordered_map<std::string, Type> stringTypes;
    protected:

        virtual Entity::ID OnPlayerJoin(Entity::ID playerId, std::string name) { return 0; };
        virtual void OnPlayerLeave(Entity::ID playerId) {};
        virtual void OnPlayerDeath(Entity::ID killer, Entity::ID victim, Entity::ID killerTeamId) {}; // Change score, check for game over, etc.
    
        void AddEvent(Event event);
        void AddEvent(Event event, Entity::ID playerTarget);

        Type type;
        Scoreboard scoreBoard;
        Util::Time::Seconds duration;
        Util::Time::Seconds respawnTime;
        uint32_t maxScore;

        std::vector<Event> broadEvents;
        std::unordered_map<Entity::ID, std::vector<Event>> targetedEvents;
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
        virtual ~TeamGameMode() {};

    protected:

        virtual Entity::ID OnPlayerJoin(Entity::ID id, std::string name) override;
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
            TeamID capturedBy;
            Util::Timer timeLeft;

            inline float GetCapturePercent()
            {
                return (timeLeft.GetElapsedTime().count() / timeToCapture.count()) * 100.0f;
            }
        };

        Domination() : TeamGameMode(Type::DOMINATION, 200, Util::Time::Seconds{60.0f * 15.0f})
        {

        }

        void Start(World world);
        void Update(World world, Util::Time::Seconds deltaTime);

        bool IsPlayerInPointArea(World world, Entity::Player* player, glm::ivec3 pos);

        std::unordered_map<glm::ivec3, PointState> pointsState;
    protected:
        Util::Timer pointsTimer;
    };

    class CaptureFlag : public TeamGameMode
    {
    public:
        static const Util::Time::Seconds timeToRecover;
        static const Util::Time::Seconds timeToReset;
        const float captureArea = 2.0f;
        const float recoverArea = 4.0f;

        struct Flag
        {
            Entity::ID flagId;
            TeamID teamId;
            std::optional<Entity::ID> carriedBy;
            glm::vec3 pos;
            glm::vec3 origin;
            Util::Timer recoverTimer;
            Util::Timer resetTimer;

            inline float GetRecoverPercent()
            {
                return (recoverTimer.GetElapsedTime().count() / timeToRecover.count()) * 100.0f;
            }
        };

        CaptureFlag() : TeamGameMode(Type::CAPTURE_THE_FLAG, 3, Util::Time::Seconds{60.0f * 12.0f}, Util::Time::Seconds{8.0f})
        {

        }

        void Start(World world) override;
        void Update(World world, Util::Time::Seconds deltaTime) override;

        bool IsPlayerInFlagArea(World world, Entity::Player* player, glm::vec3 flagPos, float area);
        bool IsFlagInOrigin(Flag& flag);

        Event GetFlagState(Flag& flag);

        std::unordered_map<Entity::ID, Flag> flags;
    protected:

        Entity::ID OnPlayerJoin(Entity::ID playerId, std::string name) override;
        void OnPlayerDeath(Entity::ID killer, Entity::ID victim, Entity::ID killerTeamId) override;
        void OnPlayerLeave(Entity::ID playerId) override;

    private:

        void SpawnFlags(World world, TeamID teamId);
        std::vector<glm::ivec3> GetCapturePoints(World world, TeamID teamId);
        void CaptureFlagBy(Entity::Player* player, Flag& flag);
        void ResetFlag(Flag& flag);
        void RecoverFlag(Flag& flag, Entity::ID playerId);
        void TakeFlag(Flag& flag, Entity::ID playerId);
        void DropFlag(Flag& flag);
        void ReturnFlag(Flag& flag);
        
        Util::Timer syncTimer{Util::Time::Seconds{3.0f}};
        Entity::ID flagId = 0;
    };
}
