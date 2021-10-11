
#include <mglogger/MGLogger.h>

#include <networking/enetw/EnetW.h>
#include <networking/Command.h>

#include <util/Time.h>
#include <util/Random.h>

#include <entity/Player.h>

#include <vector>

#include <glm/gtx/string_cast.hpp>

std::unordered_map<ENet::PeerId, Entity::Player> playerTable;
Entity::ID lastId = 0;

glm::vec3 GetRandomPlayerPosition()
{
    glm::vec3 pos;
    pos.x = Util::Random::Uniform(-7.0f, 7.0f);
    pos.y = 4.15f;
    pos.z = Util::Random::Uniform(-7.0f, 7.0f);

    return pos;
}

int main()
{
    Log::ConsoleLogger logger;
    logger.SetVerbosity(Log::Verbosity::DEBUG);

    const double TICK_RATE = 0.020;
    unsigned int tickCount = 0;

    auto hostFactory = ENet::HostFactory::Get();
    auto localhost = ENet::Address::CreateByIPAddress("127.0.0.1", 8080).value();
    auto host = hostFactory->CreateHost(localhost, 4, 2);
    host.SetOnConnectCallback([&logger, &tickCount, &host, &TICK_RATE](auto peerId)
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

        Networking::Packet::Header header;
        header.tick = tickCount;
        header.type = Networking::Command::Type::CLIENT_CONFIG;

        Networking::Packet::Payload data;
        data.config = clientConfig;

        Networking::Packet packet{header, data};

        ENet::SentPacket sentPacket{&packet, sizeof(Networking::Packet), ENET_PACKET_FLAG_RELIABLE};
        host.SendPacket(peerId, 0, sentPacket);
    });
    host.SetOnRecvCallback([&logger, TICK_RATE](auto peerId, auto channelId, ENet::RecvPacket recvPacket)
    {   
        auto packet = (Networking::Packet*)recvPacket.GetData();
        if(packet->header.type == Networking::Command::Type::PLAYER_MOVEMENT)
        {
            auto playerMove = packet->data.playerMovement;
            logger.LogInfo("Player update for peer: " + std::to_string(peerId) + ". Move dir:" + glm::to_string(playerMove.moveDir));

            const float PLAYER_SPEED = 2.f;
            auto velocity = playerMove.moveDir * PLAYER_SPEED * (float)TICK_RATE;
            auto& player = playerTable.at(peerId);
            player.transform.position += velocity;

            // TODO: This packet should be saved into a queue/buffer to later be used on the simulation, instead of using it directly.
        }
    });
    host.SetOnDisconnectCallback([&logger](auto peerId)
    {
        logger.LogInfo("Peer with id " + std::to_string(peerId) + " disconnected.");
    });

    logger.LogInfo("Server initialized. Listening on address " + localhost.GetHostName() + ":" + std::to_string(localhost.GetPort()));    
    while(true)
    {
        // TODO: This should be called until not more events are available. 
        host.PollEvent(0);

        auto now = Util::Time::GetCurrentTime();
        // TODO: Update entities with the packets recv (on queue/buffer)
        for(auto& pair : playerTable)
        {
            
        }

        auto elapsed = Util::Time::GetCurrentTime() - now;
        auto wait = (TICK_RATE - elapsed) * 1e3;

        Util::Time::SleepMS(wait);

        // Send update
        for(auto pair : playerTable)
        {
            auto player = pair.second;

            Networking::Packet::Header header;
            header.type = Networking::Command::Type::PLAYER_UPDATE;
            header.tick = tickCount;

            Networking::Packet::Payload data;
            data.playerUpdate = Networking::Command::Server::PlayerUpdate{player.id, player.transform.position};

            Networking::Packet packet {header, data};

            ENet::SentPacket epacket{&packet, sizeof(Networking::Packet), ENetPacketFlag::ENET_PACKET_FLAG_RELIABLE};
            host.Broadcast(0, epacket);
            tickCount++;
        }
    }

    logger.LogInfo("Server shutdown");

    return 0;
}