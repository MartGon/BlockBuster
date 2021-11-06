#include <HostFactory.h>

using namespace ENet;

std::unique_ptr<HostFactory> HostFactory::hostFactory_;

HostFactory* HostFactory::Get()
{
    HostFactory* ptr = hostFactory_.get();
    if(ptr == nullptr)
    {
        hostFactory_ = std::unique_ptr<HostFactory>(new HostFactory);
        ptr = hostFactory_.get();
    }

    return ptr;
}

Host HostFactory::CreateHost(Address address, uint32_t connections, uint32_t channels, uint32_t inBandwidth, uint32_t outBandwidth)
{
    return Host{address, connections, channels, inBandwidth, outBandwidth};
}

Host HostFactory::CreateHost(uint32_t connections, uint32_t channels, uint32_t inBandwidth, uint32_t outBandwidth)
{
    return Host{connections, channels, inBandwidth, outBandwidth};
}

HostFactory::HostFactory()
{
    enet_initialize();
}

HostFactory::~HostFactory()
{
    enet_deinitialize();
}