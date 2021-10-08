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

        static HostFactory* Get();

        Host CreateHost(Address address, uint32_t connections, uint32_t channels, uint32_t inBandwidth = 0, uint32_t outBandwidth = 0);
        Host CreateHost(uint32_t connections, uint32_t channels, uint32_t inBandwidth = 0, uint32_t outBandwidth = 0);

    private:
        static std::unique_ptr<HostFactory> hostFactory_;

        HostFactory();
    };
}