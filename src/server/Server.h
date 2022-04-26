#pragma once

#include <util/Ring.h>

#include <mglogger/MGLogger.h>

#include <networking/enetw/ENetW.h>
#include <networking/Snapshot.h>
#include <networking/Networking.h>
#include <networking/Packets.h>

#include <util/BBTime.h>
#include <util/Random.h>
#include <util/Ring.h>
#include <util/Table.h>

#include <collisions/Collisions.h>

#include <math/Interpolation.h>

#include <entity/Weapon.h>
#include <entity/Player.h>
#include <entity/PlayerController.h>
#include <entity/Map.h>
#include <entity/Match.h>
#include <entity/Projectile.h>

#include <http/AsyncClient.h>

#include <vector>
#include <map>

#include <ServerEvent.h>

#include <glm/gtx/string_cast.hpp>

namespace BlockBuster
{
    enum class BufferingState
    {
        REFILLING,
        CONSUMING
    };

    struct ShotCommand
    {
        ENet::PeerId clientId;
        glm::vec3 origin;
        glm::vec2 playerOrientation; // In radians
        float fov; // In radians
        float aspectRatio;
        Util::Time::Seconds commandTime;
    };

    const uint32_t MIN_INPUT_BUFFER_SIZE = 2;
    const uint32_t MAX_INPUT_BUFFER_SIZE = 5;
    
    using InputReq = Networking::Packets::Client::Input::Req;
    struct Client
    {
        ENet::PeerId id;
        std::string playerUuuid;
        std::string playerName;

        // Move this to a game class or something
        Entity::Player player;
        Entity::PlayerController pController;

        // Respawn Time
        Util::Time::Seconds respawnTime;

        // Inputs
        Util::Ring<InputReq, MAX_INPUT_BUFFER_SIZE> inputBuffer;
        uint32_t lastAck = 0;
        BufferingState state = BufferingState::REFILLING;

        // DEBUG: AI
        bool isAI = false;
        glm::vec3 targetPos;
    };

    class Server
    {
    public:

        struct Params
        {
            std::string address;
            uint16_t port;
            uint8_t maxPlayers;
            uint8_t startingPlayers;

            std::filesystem::path mapPath;
            std::string mode;

            Log::Verbosity verbosity;
        };

        struct MMServer
        {
            std::string address;
            uint16_t port;

            std::string gameId;
            std::string serverKey;
        };

        Server(Params params, MMServer mserver, Util::Time::Seconds tickRate);

        void Start();
        void Run();

    private:

        // Initialization
        void InitLogger();
        void InitNetworking();
        void InitMatch();
        void InitAI();
        void InitMap();
        void InitGameObjects();

        // Networking
        void OnClientJoin(ENet::PeerId peerId);
        void OnClientLogin(ENet::PeerId peerId, std::string playerUuid, std::string playerName);
        void OnClientLeave(ENet::PeerId peerId);
        void OnRecvPacket(ENet::PeerId peerId, uint8_t channelId, ENet::RecvPacket recvPacket);
        void OnRecvPacket(ENet::PeerId peerId, Networking::Packet& packet);
        void HandleClientInput(ENet::PeerId peerId, InputReq pm);
        void HandleClientsInput();
        void HandleShootCommand(ShotCommand sc);
        void HandleActionCommand(Entity::Player& player);
        void HandleGrenadeCommand(ENet::PeerId peerId);

        void SendWorldUpdate();
        void SendScoreboardReport();
        void SendPlayerTakeDmg(ENet::PeerId peerId, Entity::Player::HealthState health, glm::vec3 dmgOrigin);
        void SendPlayerHitConfirm(ENet::PeerId peerId, Entity::ID victimId);
        void BroadcastPlayerDied(Entity::ID killerId, Entity::ID victimId, Util::Time::Seconds respawnTime);
        void BroadcastRespawn(ENet::PeerId peerId);
        void BroadcastWarp(ENet::PeerId peerId);
        void BroadcastGameObjectState(glm::ivec3 goPos);
        void SendPlayerGameObjectInteraction(ENet::PeerId peerId, glm::ivec3 goPos);
        void SendPacket(ENet::PeerId peerId, Networking::Packet& packet);
        void Broadcast(Networking::Packet& packet);

        // Simulation
        void UpdateWorld();
        void UpdateProjectiles();
        bool IsPlayerOutOfBounds(ENet::PeerId peerId);
        void OnPlayerDeath(ENet::PeerId authorId, ENet::PeerId victimId);
        void SleepUntilNextTick(Util::Time::SteadyPoint preSimulationTime);

        // MMServer
        void SendServerNotification(ServerEvent::Notification event);

        // Spawns 
        static const float MIN_SPAWN_ENEMY_DISTANCE;
        glm::ivec3 FindSpawnPoint(Entity::Player player);
        glm::vec3 ToSpawnPos(glm::ivec3 spawnPoint);
        bool IsSpawnValid(glm::ivec3 spawnPoint, Entity::Player player);

        // Players
        void SpawnPlayer(ENet::PeerId clientId);
        void OnPlayerTakeDmg(ENet::PeerId author, ENet::PeerId victim);
        std::vector<Entity::Player> GetPlayers() const;

        // World 
        World GetWorld();
        struct GOState{
            Entity::GameObject::State state;
            Util::Timer respawnTimer;
        };
        std::unordered_map<glm::ivec3, GOState> gameObjectStates;
        Util::Table<std::unique_ptr<Entity::Projectile>> projectiles;

        // Server params
        Params params;
        MMServer mmServer;

        // Networking
        std::optional<ENet::Host> host;
        std::unordered_map<ENet::PeerId, Client> clients;
        Entity::ID lastId = 0;
        unsigned int tickCount = 0;
        Util::Ring<Networking::Snapshot, 30> history;

        // Simulation
        Util::Time::Seconds TICK_RATE{0.033};
        Util::Time::Seconds lag{0};
        Util::Time::Point<Util::Time::Seconds> nextTickDate;
        bool quit = false;

        // Match
        Match match;
        ::Game::Map::Map map;
        int lowestY = 0.0f;
        std::unordered_map<int, std::vector<glm::ivec3>> teleportOrigins;
        std::unordered_map<int, std::vector<glm::ivec3>> teleportDests;

        // Logs        
        Log::ComposedLogger logger;

        // MM Server
        HTTP::AsyncClient asyncClient;
    };
}