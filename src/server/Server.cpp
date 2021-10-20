
#include <mglogger/MGLogger.h>

#include <networking/enetw/ENetW.h>
#include <networking/Command.h>
#include <networking/CommandBuffer.h>

#include <util/Time.h>
#include <util/Random.h>

#include <entity/Player.h>

#include <vector>
#include <map>

#include <glm/gtx/string_cast.hpp>

std::unordered_map<ENet::PeerId, Entity::Player> playerTable;
std::unordered_map<ENet::PeerId, std::map<uint32_t, glm::vec3>> simHistory;
Entity::ID lastId = 0;
const double TICK_RATE = 0.020;

Networking::CommandBuffer commandBuffer{60};

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
    auto& player = playerTable[peerId];
    const float PLAYER_SPEED = 5.f;
    auto velocity = pm.moveDir * PLAYER_SPEED * (float)TICK_RATE;
    player.transform.position += velocity;
    
    simHistory[peerId][playerTick] = player.transform.position;
}

int main()
{
    Log::ConsoleLogger logger;
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
        playerTable[peerId] = player;

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
        auto command = (Networking::Command*)recvPacket.GetData();
        commandBuffer.Push(peerId, *command);
    });
    host.SetOnDisconnectCallback([&logger, &tickCount, &host](auto peerId)
    {
        logger.LogInfo("Peer with id " + std::to_string(peerId) + " disconnected.");
        auto player = playerTable[peerId];
        playerTable.erase(peerId);

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

        auto now = Util::Time::GetUNIXTime();
        std::unordered_map<ENet::PeerId, uint32_t> ackHistory;

        // Update
        for(auto& pair : playerTable)
        {
            auto peerId = pair.first;
            auto type = Networking::Command::Type::PLAYER_MOVEMENT;
            while(auto command = commandBuffer.Pop(peerId, type))
            {
                auto size = commandBuffer.GetSize(peerId, type);
                auto cmdId = command->header.tick;

                ackHistory[peerId] = cmdId;

                logger.LogInfo("Command recv with cmd id: " + std::to_string(ackHistory[peerId]) + " on tick " + std::to_string(tickCount) + ". Buffer size: " + std::to_string(size));
                HandleMoveCommand(peerId, command->data.playerMovement, tickCount);
            }

            // TODO: Save the position of the player for this tick. Slow computers will have jittery movement
            // Could use some entity interpolation on the server
        }

        auto elapsed = Util::Time::GetUNIXTime() - now;
        auto wait = (TICK_RATE - elapsed) * 1e3;
        Util::Time::SleepMS(wait);

        // Send update
        // TODO: Server commands / World Snapshots should be batched and then sent to client.
        for(auto pair : playerTable)
        {
            // Broadcast this player's movement
            auto player = pair.second;

            Networking::Command::Header header;
            header.type = Networking::Command::Type::PLAYER_POS_UPDATE;
            header.tick = tickCount;

            Networking::Command::Payload data;
            data.playerUpdate = Networking::Command::Server::PlayerUpdate{player.id, player.transform.position};

            Networking::Command command{header, data};

            ENet::SentPacket epacket{&command, sizeof(command), 0};
            host.Broadcast(0, epacket);

            // Send confirmed command to this player
            Networking::Command::Payload ackData;
            ackData.ackCommand = Networking::Command::Server::AckCommand{ackHistory[pair.first]};

            header.type = Networking::Command::Type::ACK_COMMAND;
            Networking::Command ack{header, ackData};

            ENet::SentPacket apacket{&ack, sizeof(ack), 0};
            host.SendPacket(pair.first, 0, apacket);
        }

        tickCount++;
    }

    logger.LogInfo("Server shutdown");

    return 0;
}