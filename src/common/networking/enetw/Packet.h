#pragma once

#include <enet/enet.h>

namespace ENet
{
    class RecvPacket
    {
    friend class Host;
    friend class Peer;
    public:
        
        ~RecvPacket();

        RecvPacket(const RecvPacket& ) = delete;
        RecvPacket& operator=(const RecvPacket&) = delete;

        RecvPacket(RecvPacket&&);
        RecvPacket& operator=(RecvPacket&&);

        uint32_t GetSize() const;
        const void* GetData() const;

    private:
        RecvPacket(ENetPacket* packet);
        ENetPacket* packet_ = nullptr;
    };

    class SentPacket
    {
    friend class Host;
    friend class Peer;
    public:
        SentPacket(const void* data, uint32_t size, ENetPacketFlag flag);
        
        SentPacket(const SentPacket&) = delete;
        SentPacket& operator=(const SentPacket&) = delete;

        SentPacket(SentPacket&&) = default;
        SentPacket& operator=(SentPacket&&) = default;
    private:
        const void* data_ = nullptr;
        uint32_t size_;
        ENetPacketFlag flags_;
    };
}