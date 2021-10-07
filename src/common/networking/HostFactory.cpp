#include <HostFactory.h>

using namespace ENet;

HostFactory* HostFactory::Get()
{
    HostFactory* ptr = socketFactory_.get();
    if(ptr == nullptr)
    {
        socketFactory_ = std::unique_ptr<HostFactory>();
        ptr = socketFactory_.get();
    }

    return ptr;
}

Host HostFactory::CreateHost(Address address, uint32_t connections, uint32_t channels, uint32_t inBandwidth, uint32_t outBandwidth)
{
    return Host{address, connections, channels, inBandwidth, outBandwidth};
}

HostFactory::HostFactory()
{
    enet_initialize();
}

HostFactory::~HostFactory()
{
    enet_deinitialize();
}