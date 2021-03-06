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

        bool SendPacket(uint8_t channelId, const SentPacket& packet);

    private:
        Peer(ENetPeer* peer);

        void Disconnect();

        ENetPeer* peer_ = nullptr;
    };
}