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

#include <collisions/Collisions.h>

#include <math/Interpolation.h>

#include <entity/Weapon.h>
#include <entity/Player.h>
#include <entity/PlayerController.h>
#include <entity/Map.h>

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
        std::string playerUuuid;
        std::string playerName;

        // Move this to a game class or something
        Entity::Player player;
        Entity::PlayerController pController;

        Util::Ring<InputReq, MAX_INPUT_BUFFER_SIZE> inputBuffer;
        Util::Ring<ShotCommand, MAX_INPUT_BUFFER_SIZE> shotBuffer;
        uint32_t lastAck = 0;
        BufferingState state = BufferingState::REFILLING;

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

        Server(Params params, MMServer mserver);

        void Start();
        void Run();

    private:

        // Initialization
        void InitLogger();
        void InitNetworking();
        void InitAI();
        void InitMap();

        // Networking
        void OnClientJoin(ENet::PeerId peerId);
        void OnClientLeave(ENet::PeerId peerId);
        void OnRecvPacket(ENet::PeerId peerId, uint8_t channelId, ENet::RecvPacket recvPacket);
        void OnRecvPacket(ENet::PeerId peerId, Networking::Packet& packet);
        void HandleClientInput(ENet::PeerId peerId, InputReq pm);
        void HandleClientsInput();
        void HandleShootCommand(ShotCommand sc);
        void SendWorldUpdate();
        void SendPlayerTakeDmg(ENet::PeerId peerId, Entity::Player::HealthState health, glm::vec3 dmgOrigin);

        // Simulation
        void SleepUntilNextTick(Util::Time::SteadyPoint preSimulationTime);

        // MMServer
        void SendServerNotification(ServerEvent::Notification event);

        // Spawns 
        // TODO: Should move these to their class/module
        static const float MIN_SPAWN_ENEMY_DISTANCE;
        glm::ivec3 FindSpawnPoint(Entity::Player player);
        glm::vec3 ToSpawnPos(glm::ivec3 spawnPoint);
        std::vector<Entity::Player> GetPlayers() const;
        bool IsSpawnValid(glm::ivec3 spawnPoint, Entity::Player player) const;

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
        const Util::Time::Seconds TICK_RATE{0.050};
        Util::Time::Seconds deltaTime{0};
        Util::Time::Seconds lag{0};
        Util::Time::Point<Util::Time::Seconds> nextTickDate;

        // World
        Game::Map::Map map;

        // Logs        
        Log::ComposedLogger logger;

        // MM Server
        HTTP::AsyncClient asyncClient;
    };
}