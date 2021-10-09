
#include <mglogger/MGLogger.h>

#include <networking/enetw/EnetW.h>
#include <networking/NetworkPacket.h>

#include <math/Transform.h>
#include <util/Time.h>

#include <vector>

#include <glm/gtx/string_cast.hpp>

int main()
{
    Log::ConsoleLogger logger;
    logger.SetVerbosity(Log::Verbosity::INFO);

    std::unordered_map<uint8_t, glm::vec3> players ={
        {1, glm::vec3{-7.0f, 4.15f, -7.0f}}
    };

    const double TICK_RATE = 0.020;

    auto hostFactory = ENet::HostFactory::Get();
    auto localhost = ENet::Address::CreateByIPAddress("127.0.0.1", 8080).value();
    auto host = hostFactory->CreateHost(localhost, 4, 2);
    host.SetOnConnectCallback([&logger](auto peerId)
    {
        logger.LogInfo("Connected with peer " + std::to_string(peerId));
    });
    host.SetOnRecvCallback([&logger, &players, TICK_RATE](auto peerId, auto channelId, ENet::RecvPacket recvPacket)
    {
        
        auto packet = (Networking::Packet*)recvPacket.GetData();
        if(packet->header.type == Networking::Packet::Type::PLAYER_MOVEMENT)
        {
            auto playerMove = packet->data.playerMovement;
            logger.LogInfo("Player update for peer: " + std::to_string(peerId) + ". Move dir:" + glm::to_string(playerMove.moveDir));

            const float PLAYER_SPEED = 2.f;
            auto velocity = playerMove.moveDir * PLAYER_SPEED * (float)TICK_RATE;
            players.at(1) += velocity;
        }
    });
    host.SetOnDisconnectCallback([&logger](auto peerId)
    {
        logger.LogInfo("Peer with id " + std::to_string(peerId) + " disconnected.");
    });

    unsigned int tickCount = 0;
    logger.LogInfo("Server initialized. Listening on address " + localhost.GetHostName() + ":" + std::to_string(localhost.GetPort()));

    
    while(true)
    {
        host.PollEvent(0);

        auto now = Util::Time::GetCurrentTime();
        // Update entities with the packets recv
        for(auto& pair : players)
        {
            
        }

        auto elapsed = Util::Time::GetCurrentTime() - now;
        auto wait = (TICK_RATE - elapsed) * 1e3;

        Util::Time::SleepMS(wait);

        // Send update
        for(auto pair : players)
        {
            Networking::Packet::Header header;
            header.type = Networking::Packet::Type::PLAYER_UPDATE;
            header.tick = tickCount;

            Networking::Payload::Data data;
            data.playerUpdate = Networking::Payload::PlayerUpdate{pair.first, pair.second};

            Networking::Packet packet {header, data};

            ENet::SentPacket epacket{&packet, sizeof(Networking::Packet), ENetPacketFlag::ENET_PACKET_FLAG_RELIABLE};
            host.Broadcast(0, epacket);
            tickCount++;
        }
    }

    logger.LogInfo("Server shutdown");

    return 0;
}