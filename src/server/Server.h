#pragma once

#include <util/Ring.h>

#include <mglogger/MGLogger.h>

#include <networking/enetw/ENetW.h>
#include <networking/Command.h>
#include <networking/Snapshot.h>
#include <networking/Networking.h>

#include <util/BBTime.h>
#include <util/Random.h>
#include <util/Ring.h>

#include <collisions/Collisions.h>

#include <math/Interpolation.h>

#include <entity/Player.h>

#include <vector>
#include <map>

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
        Networking::Command::User::PlayerShot playerShot;
        Util::Time::Seconds commandTime;
    };

    const uint32_t MIN_INPUT_BUFFER_SIZE = 2;
    const uint32_t MAX_INPUT_BUFFER_SIZE = 5;
    struct Client
    {
        Entity::Player player;
        Util::Ring<Networking::Command::User::PlayerMovement, MAX_INPUT_BUFFER_SIZE> inputBuffer;
        Util::Ring<ShotCommand, MAX_INPUT_BUFFER_SIZE> shotBuffer;
        uint32_t lastAck = 0;
        BufferingState state = BufferingState::REFILLING;

        bool isAI = false;
        glm::vec3 targetPos;
    };

    class Server
    {
    public:

        void Start();
        void Run();

    private:

        // Initialization
        void InitLogger();
        void InitNetworking();
        void InitAI();

        // Networking
        void HandleMoveCommand(ENet::PeerId peerId, Networking::Command::User::PlayerMovement pm);
        void HandleClientsInput();
        void HandleShootCommand(BlockBuster::ShotCommand shotCmd);

        // Misc
        glm::vec3 GetRandomPos() const;
        

        const Util::Time::Seconds TICK_RATE{0.050};
        const float PLAYER_SPEED = 5.f;

        std::optional<ENet::Host> host;
        std::unordered_map<ENet::PeerId, Client> clients;
        Entity::ID lastId = 0;
        unsigned int tickCount = 0;
        Util::Ring<Networking::Snapshot, 30> history; 

        Log::ComposedLogger logger;
    };
}