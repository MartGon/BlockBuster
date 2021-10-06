#pragma once

#include <enet/enet.h>

#include <Packet.h>

namespace ENet
{
    class Peer
    {
    friend class Host;
    public:
        ~Peer();

        Peer(const Peer&) = delete;
        Peer& operator=(const Peer&) = delete;

        Peer(Peer&&);
        Peer& operator=(Peer&&);

        bool SendPacket(uint8_t channelId, const Packet& packet);

    private:
        Peer(ENetPeer* peer);

        ENetPeer* peer_;
    };
}