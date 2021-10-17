#include <Host.h>
#include <Exception.h>

#include <chrono>

using namespace ENet;

Host::Host(Address address, uint32_t connections, uint32_t channels, uint32_t inBandwidth, uint32_t outBandwidth) : 
    address_{address}, connections_{connections}, channels_{channels}, inBandwidth_{inBandwidth}, outBandwidth_{outBandwidth}
{
    socket_ = enet_host_create(&address.address_, connections, channels, inBandwidth, outBandwidth);
    if(socket_ == nullptr)
        throw Exception("Could not create Enet::Host");
}

Host::Host(uint32_t connections, uint32_t channels, uint32_t inBandwidth, uint32_t outBandwidth) : 
    address_{Address::CreateNull()}, connections_{connections}, channels_{channels}, inBandwidth_{inBandwidth}, outBandwidth_{outBandwidth}
{
    socket_ = enet_host_create(nullptr, connections, channels, inBandwidth, outBandwidth);
    if(socket_ == nullptr)
        throw Exception("Could not create Enet::Host");
}

Host::~Host()
{
    if(socket_)
        enet_host_destroy(socket_);
}

Host::Host(Host&& other) : address_{other.address_}
{
    *this = std::move(other);
}

Host& Host::operator=(Host&& other)
{
    std::swap(socket_, other.socket_);
    address_ = other.address_;
    channels_ = other.channels_;
    inBandwidth_ = other.inBandwidth_;
    outBandwidth_ = other.outBandwidth_;

    return *this;
}

void Host::SetOnRecvCallback(std::function<void(PeerId, uint8_t, RecvPacket)> callback)
{
    onRecv_ = callback;
}

void Host::SetOnConnectCallback(std::function<void(PeerId)> callback)
{
    onConnect_ = callback;
}

void Host::SetOnDisconnectCallback(std::function<void(PeerId)> callback)
{
    onDisconnect_ = callback;
}

bool Host::PollEvent(uint32_t timeout)
{
    ENetEvent event;
    bool eventOccurred = enet_host_service(socket_, &event, timeout) > 0;

    switch (event.type)
    {
        case ENET_EVENT_TYPE_CONNECT:
        {
            auto peerId = AddPeer(event.peer);
            if(onConnect_)
                onConnect_(peerId);
            break;
        }
        case ENET_EVENT_TYPE_RECEIVE:
        {
            RecvPacket recv{event.packet};
            auto peerId = GetPeerId(event.peer);
            if(onRecv_)
                onRecv_(peerId, event.channelID, std::move(recv));
            break;
        }
        case ENET_EVENT_TYPE_DISCONNECT:
        {
            auto peerId = GetPeerId(event.peer);
            if(onDisconnect_)
                onDisconnect_(peerId);
            RemovePeer(event.peer);
            break;
        }
        default:
            break;
    }

    return eventOccurred;
}

void Host::PollAllEvents(uint32_t timeout)
{
    using namespace std::chrono;
    auto start = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    while(PollEvent())
    {
        auto now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        auto elapsed = now - start;

        if(elapsed > timeout)
            break;
    }
}

void Host::Connect(Address address)
{
    enet_host_connect(socket_, &address.address_, channels_, 0);
}

void Host::SendPacket(PeerId id, uint8_t channelId, const SentPacket& packet)
{
    auto p = enet_packet_create(packet.data_, packet.size_, packet.flags_);
    auto& peer = GetPeer(id);
    peer.SendPacket(channelId, packet);
}

void Host::Broadcast(uint8_t channelId, const SentPacket& packet)
{
    auto p = enet_packet_create(packet.data_, packet.size_, packet.flags_);
    enet_host_broadcast(socket_, channelId, p);
}

PeerInfo Host::GetPeerInfo(PeerId id) const
{
    auto& peer = GetPeer(id);

    PeerInfo info;
    info.address = Address{peer.peer_->address};
    info.roundTripTimeMs = peer.peer_->roundTripTime;
    info.roundTripTimeVariance = peer.peer_->roundTripTimeVariance;
    info.packetsSent = peer.peer_->packetsSent;
    info.packetsAck = enet_list_size(&peer.peer_->acknowledgements);
    info.packetsLost = peer.peer_->packetsLost;
    info.packetLoss = peer.peer_->packetLoss;
    info.incomingBandwidth = peer.peer_->incomingBandwidth;
    info.outgoingBandwidth = peer.peer_->outgoingBandwidth;

    return info;
}

// ##### PRIVATE ##### \\

const Peer& Host::GetPeer(PeerId id) const
{
    CheckPeer(id);
    return peers_.at(id);
}

Peer& Host::GetPeer(PeerId id)
{
    CheckPeer(id);
    return peers_.at(id);
}

PeerId Host::AddPeer(ENetPeer* peer)
{
    auto peerId = GetFreePeerId();
    if(peerId != (uint8_t)-1)
        peers_.emplace(peerId, Peer{peer});
    else
        throw Exception("Could not add Peer. Max connections reached.");

    return peerId;
}

PeerId Host::RemovePeer(ENetPeer* peer)
{
    auto peerId = GetPeerId(peer);
    peers_.erase(peerId);
    return peerId;
}

void Host::CheckPeer(PeerId id) const
{
    if(peers_.find(id) == peers_.end())
        throw Exception("Could not find peer with id " + std::to_string(id));
}

PeerId Host::GetPeerId(ENetPeer* peer) const
{
    for(auto& pair : peers_)
        if(pair.second.peer_ == peer)
            return pair.first;
    
    return -1;
}

PeerId Host::GetFreePeerId() const
{
    for(uint32_t i = 0; i < connections_; i++)
        if(peers_.find(i) == peers_.end())
            return i;

    return -1;
}