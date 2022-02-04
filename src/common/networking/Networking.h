#pragma once

#include <stdint.h>
#include <vector>
#include <memory>
#include <unordered_map>

#include <entity/Player.h>
#include <Snapshot.h>

#include <util/Buffer.h>


namespace Networking
{
    enum OpcodeClient : uint16_t
    {
        INPUT
    };

    enum OpcodeServer : uint16_t
    {
        WELCOME,
        SNAPSHOT,
        PLAYER_DISCONNECTED
    };

    enum class PacketType : uint8_t
    {
        Server,
        Client
    };

    class Packet
    {
    public:
        Packet(uint16_t opCode) : opCode{opCode}
        {

        }

        Packet(uint16_t opCode, Util::Buffer&& buffer) : buffer{std::move(buffer)}, opCode{opCode}
        {

        }

        virtual ~Packet()
        {
            
        }

        inline uint32_t GetSize()
        {
            return buffer.GetSize();
        }

        inline uint16_t GetOpcode()
        {
            return opCode;
        }

        inline Util::Buffer* GetBuffer()
        {
            return &buffer;
        }

        inline void SetBuffer(Util::Buffer&& buffer)
        {
            this->buffer = std::move(buffer);
        }

        template<typename T>
        inline T* To()
        {
            return static_cast<T*>(this);
        }

        void Read();
        void Write();

        virtual void OnRead(Util::Buffer::Reader reader) = 0;
        virtual void OnWrite() = 0;

    protected:
        uint16_t opCode;
        Util::Buffer buffer;
    };

    template <PacketType type>
    std::unique_ptr<Packet> MakePacket(uint16_t opCode);

    class Batch
    {
    public:

        template <PacketType Type>
        void Read()
        {
            auto reader = buffer.GetReader();

            while(!reader.IsOver())
            {
                auto len = reader.Read<uint32_t>();
                auto opCode = reader.Read<uint16_t>();

                auto packetBuffer = reader.Read(len - sizeof(opCode));
                std::unique_ptr<Packet> packet = MakePacket<Type>(opCode);
                packet->SetBuffer(std::move(packetBuffer));
                packet->Read();

                packets.push_back(std::move(packet));
            }
        }

        void Write();

        inline uint32_t GetPacketCount()
        {
            return packets.size();
        }

        inline Packet* GetPacket(uint32_t index)
        {
            Packet* packet = nullptr;
            if(index < GetPacketCount())
                packet = packets[index].get();
            return packet;
        }

        inline void PushPacket(std::unique_ptr<Packet>&& packet)
        {
            packets.push_back(std::move(packet));
        }
        
        inline Util::Buffer* GetBuffer()
        {
            return &buffer;
        }

        inline void SetBuffer(Util::Buffer&& buffer)
        {
            this->buffer = std::move(buffer);
        }

    private:
        std::vector<std::unique_ptr<Packet>> packets;

        Util::Buffer buffer;
    };

    namespace Packets
    {
        namespace Server
        {
            class Welcome final : public Packet
            {
            public:
                Welcome() : Packet{OpcodeServer::WELCOME}
                {

                }
                void OnRead(Util::Buffer::Reader reader) override;
                void OnWrite() override;

                uint8_t playerId;
                double tickRate;
            };

            class WorldUpdate final : public Packet
            {
            public:
                WorldUpdate() : Packet{OpcodeServer::SNAPSHOT}
                {

                }
                void OnRead(Util::Buffer::Reader reader) override;
                void OnWrite() override;

                uint32_t lastCmd = 0;
                Snapshot snapShot;
            };
            
            class PlayerDisconnected final : public Packet
            {
            public:
                void OnRead(Util::Buffer::Reader reader) override;
                void OnWrite() override;

            private:
            };
        }

        namespace Client
        {
            class Input final : public Packet
            {   
            public:
                Input() : Packet{OpcodeClient::INPUT}
                {

                }
                void OnRead(Util::Buffer::Reader reader) override;
                void OnWrite() override;

                uint32_t reqId;
                Entity::PlayerInput playerInput;
            };
        }
    }
}