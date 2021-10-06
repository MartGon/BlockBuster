#include <Packet.h>

#include <Exception.h>

using namespace ENet;

Packet::Packet(const void* data, uint32_t size, ENetPacketFlag flags)
{
    packet_ = enet_packet_create(data, size, flags);
    if(packet_ == nullptr)
        throw Exception("Could not create packet");
}

Packet::~Packet()
{
    enet_packet_destroy(packet_);
}

Packet::Packet(Packet&& other)
{
    *this = std::move(other);
}

Packet& Packet::operator=(Packet&& other)
{
    std::swap(packet_, other.packet_);
    return *this;
}