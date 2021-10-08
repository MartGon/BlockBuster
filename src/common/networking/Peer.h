#pragma once

#include <enet/enet.h>

#include <Packet.h>

namespace ENet
{
    class Peer
    {
    friend class Host;
    public:
        // TODO: Move back to private. Only hosts should create these
        Peer(ENetPeer* peer);
        ~Peer();

        Peer(const Peer&) = delete;
        Peer& operator=(const Peer&) = delete;

        Peer(Peer&&);
        Peer& operator=(Peer&&);

        bool SendPacket(uint8_t channelId, const Packet& packet);

        // Get address

    private:

        ENetPeer* peer_ = nullptr;
    };
}