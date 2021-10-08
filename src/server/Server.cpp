
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

    logger.LogInfo("Server initialized. Listening on address " + localhost.GetHostName() + ":" + std::to_string(localhost.GetPort()));

    unsigned int tickCount = 0;
    std::vector<ENet::Peer> peers;
    while(true)
    {
        auto event = host.PollEvent(1000);
        switch (event.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
            logger.LogInfo("A new client connected");
            peers.emplace_back(event.peer);
            break;
        case ENET_EVENT_TYPE_RECEIVE:
        {
            logger.LogInfo("Client packet recv of size: " + std::to_string(event.packet->dataLength));
            const char *str = (const char*) event.packet->data;
            logger.LogInfo("Client packet with data: " + std::string(str));
            break;
        }
        case ENET_EVENT_TYPE_DISCONNECT:
            logger.LogInfo("Peer disconnected");
            break;
        
        default:
            //logger.LogInfo("Nothing happened"));
            break;
        }

        for(auto& peer : peers)
        {
            ENet::Packet packet{&tickCount, sizeof(tickCount), ENET_PACKET_FLAG_RELIABLE};
            peer.SendPacket(0, packet);
        }

        tickCount++;
    }

    logger.LogInfo("Server shutdown");

    return 0;
}