#include <Peer.h>

#include <utility>

using namespace ENet;

Peer::Peer(ENetPeer* peer) : peer_{peer}
{

}

Peer::~Peer()
{
}

Peer::Peer(Peer&& other)
{
    *this = std::move(other);
}

Peer& Peer::operator=(Peer&& other)
{
    std::swap(peer_, other.peer_);
    return *this;
}

bool Peer::SendPacket(uint8_t channelId, const Packet& packet)
{
    return enet_peer_send(peer_, channelId, packet.packet_) == 0;
}