#include <Packet.h>

#include <Exception.h>

using namespace ENet;

RecvPacket::RecvPacket(ENetPacket* packet) : packet_{packet}
{
}

RecvPacket::~RecvPacket()
{
    enet_packet_destroy(packet_);
}

RecvPacket::RecvPacket(RecvPacket&& other)
{
    *this = std::move(other);
}

RecvPacket& RecvPacket::operator=(RecvPacket&& other)
{
    std::swap(packet_, other.packet_);
    return *this;
}

uint32_t RecvPacket::GetSize() const
{
    return packet_->dataLength;
}

const void* RecvPacket::GetData() const
{
    return packet_->data;
}

SentPacket::SentPacket(const void* data, uint32_t size, ENetPacketFlag flags) : 
    data_{data}, size_{size}, flags_{flags}
{
}