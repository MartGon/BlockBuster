#pragma once

#include <stdint.h>
#include <optional>

#include <Peer.h>
#include <Address.h>

namespace ENet
{
    class Host
    {
    friend class HostFactory;
    public:
        ~Host();

        Host(const Host&) = delete;
        Host& operator=(const Host&) = delete;

        Host(Host&&);
        Host& operator=(Host&&);

        ENetEvent PollEvent(uint32_t timeout = 0);
        std::optional<ENet::Peer> Connect(Address address);
        void Broadcast(uint8_t channelId, const Packet& packet);

    private:
        Host(Address address, uint32_t connections, uint32_t channels, uint32_t inBandwidth, uint32_t outBandwidth);

        Address address_;
        uint32_t connections_;
        uint32_t channels_;
        uint32_t inBandwidth_;
        uint32_t outBandwidth_;
        ENetHost* socket_ = nullptr;
    };
}