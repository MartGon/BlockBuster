#pragma once

#include <enet/enet.h>
#include <memory>
#include <optional>

#include <Host.h>

namespace ENet
{
    class HostFactory
    {
    public:
        ~HostFactory();

        HostFactory* Get();

        Host CreateHost(Address address, uint32_t connections, uint32_t channels, uint32_t inBandwidth, uint32_t outBandwidth);

    private:
        static std::unique_ptr<HostFactory> socketFactory_;

        HostFactory();
    };
}