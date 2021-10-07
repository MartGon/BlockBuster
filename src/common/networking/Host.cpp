#include <Host.h>
#include <Exception.h>

using namespace ENet;

Host::Host(Address address, uint32_t connections, uint32_t channels, uint32_t inBandwidth, uint32_t outBandwidth) : 
    address_{address}, connections_{connections}, channels_{channels}, inBandwidth_{inBandwidth}, outBandwidth_{outBandwidth}
{
    socket_ = enet_host_create(&address.address_, connections, channels, inBandwidth, outBandwidth);
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

ENetEvent Host::PollEvent(uint32_t tiemout)
{
    ENetEvent event;
    enet_host_service(socket_, &event, tiemout);

    return event;
}

std::optional<Peer> Host::Connect(Address address)
{
    std::optional <Peer> ret;
    auto peer = enet_host_connect(socket_, &address.address_, channels_, 0);
    if(peer)
        ret = Peer{peer};

    return ret;
}

void Host::Broadcast(uint8_t channelId, const Packet& packet)
{
    enet_host_broadcast(socket_, channelId, packet.packet_);
}