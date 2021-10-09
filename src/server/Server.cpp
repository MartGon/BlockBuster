
#include <mglogger/MGLogger.h>
#include <networking/HostFactory.h>

#include <vector>

int main()
{
    Log::ConsoleLogger logger;
    logger.SetVerbosity(Log::Verbosity::INFO);

    auto hostFactory = ENet::HostFactory::Get();
    auto localhost = ENet::Address::CreateByIPAddress("127.0.0.1", 8080).value();
    auto host = hostFactory->CreateHost(localhost, 4, 2);

    host.SetOnConnectCallback([&logger](auto peerId)
    {
        logger.LogInfo("Connected with peer " + std::to_string(peerId));
    });

    host.SetOnDisconnectCallback([&logger](auto peerId)
    {
        logger.LogInfo("Peer with id " + std::to_string(peerId) + " disconnected.");
    });

    unsigned int tickCount = 0;
    logger.LogInfo("Server initialized. Listening on address " + localhost.GetHostName() + ":" + std::to_string(localhost.GetPort()));
    while(tickCount < 30)
    {
        host.PollEvent(1000);

        ENet::SentPacket packet{&tickCount, 4, ENetPacketFlag::ENET_PACKET_FLAG_RELIABLE};
        host.Broadcast(0, packet);
        tickCount++;
    }

    logger.LogInfo("Server shutdown");

    return 0;
}