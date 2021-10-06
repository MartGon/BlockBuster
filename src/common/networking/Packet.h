#pragma once

#include <enet/enet.h>

namespace ENet
{
    class Packet
    {
    friend class Host;
    friend class Peer;
    public:
        Packet(const void* data, uint32_t size, ENetPacketFlag flags);
        ~Packet();

        Packet(const Packet& ) = delete;
        Packet& operator=(const Packet&) = delete;

        Packet(Packet&&);
        Packet& operator=(Packet&&);

    private:
        ENetPacket* packet_;
    };
}