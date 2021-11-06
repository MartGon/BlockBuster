
#include <mglogger/MGLogger.h>

#include <networking/enetw/ENetW.h>
#include <networking/Command.h>
#include <networking/CommandBuffer.h>
#include <networking/Snapshot.h>

#include <util/Time.h>
#include <util/Random.h>

#include <entity/Player.h>

#include <vector>
#include <map>

#include <glm/gtx/string_cast.hpp>

const uint32_t MIN_INPUT_BUFFER_SIZE = 2;
const uint32_t MAX_INPUT_BUFFER_SIZE = 5;

enum class BufferingState
{
    REFILLING,
    CONSUMING
};

struct Client
{
    Entity::Player player;
    Util::Ring<Networking::Command, MAX_INPUT_BUFFER_SIZE> inputBuffer;
    uint32_t lastAck = 0;
    BufferingState state = BufferingState::REFILLING;
};

std::unordered_map<ENet::PeerId, Client> clients;
Entity::ID lastId = 0;
const double TICK_RATE = 0.050;
const double UPDATE_RATE = 0.050;
const float PLAYER_SPEED = 5.f;

glm::vec3 GetRandomPlayerPosition()
{
    glm::vec3 pos;
    pos.x = Util::Random::Uniform(-7.0f, 7.0f);
    pos.y = 4.15f;
    pos.z = Util::Random::Uniform(-7.0f, 7.0f);

    return pos;
}

void HandleMoveCommand(ENet::PeerId peerId, Networking::Command::User::PlayerMovement pm, uint32_t playerTick)
{
    auto& player = clients[peerId].player;
    auto len = glm::length(pm.moveDir);
    if(len > 0)
    {
        auto moveDir = glm::normalize(pm.moveDir);
        auto velocity = pm.moveDir * PLAYER_SPEED * (float)TICK_RATE;
        player.transform.position += velocity;
    }
}

int main()
{
    // Loggers
    auto clogger = std::make_unique<Log::ConsoleLogger>();
    auto flogger = std::make_unique<Log::FileLogger>();
    std::filesystem::path logFile = "server.log";
    flogger->OpenLogFile("server.log");
    if(!flogger->IsOk())
        clogger->LogError("Could not create log file " + logFile.string());

    Log::ComposedLogger logger;
    logger.AddLogger(std::move(clogger));
    logger.AddLogger(std::move(flogger));
    logger.SetVerbosity(Log::Verbosity::DEBUG);

    unsigned int tickCount = 0;

    auto hostFactory = ENet::HostFactory::Get();
    auto localhost = ENet::Address::CreateByIPAddress("127.0.0.1", 8081).value();
    auto host = hostFactory->CreateHost(localhost, 4, 2);
    host.SetOnConnectCallback([&logger, &tickCount, &host](auto peerId)
    {
        logger.LogInfo("Connected with peer " + std::to_string(peerId));

        // Add player to table
        Entity::Player player;
        player.id = lastId++;
        player.transform = Math::Transform{GetRandomPlayerPosition(), glm::vec3{0.0f}, glm::vec3{1.0f}};
        clients[peerId].player = player;

        // Inform client
        Networking::Command::Server::ClientConfig clientConfig;
        clientConfig.playerId = player.id;
        clientConfig.sampleRate = TICK_RATE;

        Networking::Command::Header header;
        header.tick = tickCount;
        header.type = Networking::Command::Type::CLIENT_CONFIG;

        Networking::Command::Payload data;
        data.config = clientConfig;

        Networking::Command packet{header, data};

        ENet::SentPacket sentPacket{&packet, sizeof(packet), ENET_PACKET_FLAG_RELIABLE};
        host.SendPacket(peerId, 0, sentPacket);
    });
    host.SetOnRecvCallback([&logger, &tickCount](auto peerId, auto channelId, ENet::RecvPacket recvPacket)
    {   
        auto& client = clients[peerId];
        auto command = (Networking::Command*)recvPacket.GetData();

        // FIXME/TODO: Check if the packet to be push has a cmdId > lastACK. Drop really delayed packets
        // FIXME/TODO: Sort after push. Use a prio queue. Mitigates unordered packets
        // FIXME/TODO: Extract all redundant inputs from the packet
        client.inputBuffer.PushBack(*command);
        logger.LogInfo("Command arrived with cmdid " + std::to_string(command->header.tick));

        // FIXME/TODO: If this happens, then the client is producing more often than the server is consuming
        auto bufferSize = client.inputBuffer.GetSize();
        if(bufferSize >= MAX_INPUT_BUFFER_SIZE)
            logger.LogInfo("Max Buffer size reached." + std::to_string(bufferSize));

        if(client.state == BufferingState::REFILLING && client.inputBuffer.GetSize() > MIN_INPUT_BUFFER_SIZE)
            client.state = BufferingState::CONSUMING;
    });
    host.SetOnDisconnectCallback([&logger, &tickCount, &host](auto peerId)
    {
        logger.LogInfo("Peer with id " + std::to_string(peerId) + " disconnected.");
        auto player = clients[peerId].player;
        clients.erase(peerId);

        // Informing players
        Networking::Command::Server::PlayerDisconnected playerDisconnect;
        playerDisconnect.playerId = player.id;

        Networking::Command::Header header;
        header.tick = tickCount;
        header.type = Networking::Command::Type::PLAYER_DISCONNECTED;

        Networking::Command::Payload payload;
        payload.playerDisconnect = playerDisconnect;

        Networking::Command command{header, payload};

        ENet::SentPacket sentPacket{&command, sizeof(command), ENetPacketFlag::ENET_PACKET_FLAG_RELIABLE};
        host.Broadcast(0, sentPacket);
    });

    logger.LogInfo("Server initialized. Listening on address " + localhost.GetHostName() + ":" + std::to_string(localhost.GetPort()));    
    while(true)
    {
        host.PollAllEvents();

        auto now = Util::Time::GetTime();

        // Handle client inputs
        for(auto& pair : clients)
        {
            auto peerId = pair.first;
            auto& client = pair.second;
            
            // FIXME/TODO: Assume for now that packets don't arrive in incorrect order
            if(client.state == BufferingState::CONSUMING)
            {
                if(auto command = client.inputBuffer.PopFront(); command.has_value())
                {
                    auto cmdId = command->header.tick;
                    client.lastAck = cmdId;

                    logger.LogInfo("Cmd id: " + std::to_string(client.lastAck) + " from " + std::to_string(pair.first));
                    HandleMoveCommand(peerId, command->data.playerMovement, tickCount);

                    logger.LogInfo("Ring size " + std::to_string(client.inputBuffer.GetSize()));
                }
                else
                {
                    logger.LogWarning("No cmd handled for player " + std::to_string(pair.first) + " waiting for buffer to refill ");
                    client.state = BufferingState::REFILLING;
                }
            }

            // TODO: Save the position of the player for this tick. Slow computers will have jittery movement
            // Could use some entity interpolation on the server
        }

        // Create snapshot
        Networking::Snapshot s;
        s.serverTick = tickCount;
        for(auto pair : clients)
        {
            auto player = pair.second.player;
            s.players[player.id].pos = player.transform.position;
        }

        auto snapshotBuffer = s.ToBuffer();

        // Send snapthot with ack
        for(auto pair : clients)
        {
            auto client = pair.second;
            Util::Buffer buffer;

            Networking::Command::Header header;
            header.type = Networking::Command::Type::SERVER_SNAPSHOT;
            header.tick = tickCount;

            Networking::Command::Server::Update update;
            update.lastCmd = client.lastAck;
            update.snapshotDataSize = snapshotBuffer.GetSize();

            Networking::Command::Payload snapshotData;
            snapshotData.snapshot = update;

            Networking::Command cmd{header, snapshotData};

            Util::Buffer packetBuf;
            packetBuf.Write(cmd);
            
            packetBuf = Util::Buffer::Concat(std::move(packetBuf), snapshotBuffer.Clone());

            ENet::SentPacket epacket{packetBuf.GetData(), packetBuf.GetSize(), 0};
            host.SendPacket(pair.first, 0, epacket);
        }

        auto elapsed = Util::Time::GetTime() - now;
        auto wait = Util::Time::Seconds{TICK_RATE} - elapsed;
        logger.LogInfo("Job done. Sleeping during " + std::to_string(Util::Time::Seconds{wait}.count()));
        logger.Flush();
        Util::Time::Sleep(wait);

        tickCount++;
    }

    logger.LogInfo("Server shutdown");

    return 0;
}