#pragma once


#include <memory>

#include <networking/Networking.h>
#include <entity/Player.h>
#include <Snapshot.h>

namespace Networking
{
    // NOTES: How to properly add a new packet type
    //  1. Define its type in one of the enums
    //  2. Define its class in the proper namespace. Implement OnRead and OnWrite
    //  3. Create proper case in the MakePacket function
    //  4. Handle its case in Server::OnRecv or Cleint::OnRecv

    enum OpcodeServer : uint16_t
    {
        OPCODE_SERVER_BATCH,
        OPCODE_SERVER_WELCOME,
        OPCODE_SERVER_SNAPSHOT,
        OPCODE_SERVER_PLAYER_DISCONNECTED,
        OPCODE_SERVER_PLAYER_INFO
    };

    enum OpcodeClient : uint16_t
    {
        OPCODE_CLIENT_BATCH,
        OPCODE_CLIENT_LOGIN,
        OPCODE_CLIENT_INPUT,
    };

    enum class PacketType : uint8_t
    {
        Server,
        Client
    };

    template <PacketType type>
    std::unique_ptr<Packet> MakePacket(uint16_t opCode);

    template <PacketType type>
    std::unique_ptr<Packet> MakePacket(Util::Buffer&& buffer)
    {
        auto reader = buffer.GetReader();
        auto opCode = reader.Read<uint16_t>();

        auto packet = MakePacket<type>(opCode);
        if(packet)
            packet->SetBuffer(std::move(buffer));
        return std::move(packet);
    }

    template <PacketType Type>
    class Batch : public Packet
    {
    public:
        Batch();

        void OnRead(Util::Buffer::Reader reader) override
        {
            while(!reader.IsOver())
            {
                auto len = reader.Read<uint32_t>();

                auto packetBuffer = reader.Read(len);
                std::unique_ptr<Packet> packet = MakePacket<Type>(std::move(packetBuffer));
                packet->Read();

                packets.push_back(std::move(packet));
            }
        }

        void OnWrite() override
        {
            //buffer.Clear();

            for(auto& packet : packets)
            {
                packet->Write();
                buffer.Write(packet->GetSize());
                buffer.Append(std::move(*packet->GetBuffer()));
            }
        }

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

    private:
        std::vector<std::unique_ptr<Packet>> packets;
    };

    namespace Packets
    {
        namespace Server
        {
            class Welcome final : public Packet
            {
            public:
                Welcome() : Packet{OpcodeServer::OPCODE_SERVER_WELCOME}
                {

                }
                void OnRead(Util::Buffer::Reader reader) override;
                void OnWrite() override;

                uint8_t playerId;
                double tickRate;
                Entity::PlayerState playerState;
            };

            class WorldUpdate final : public Packet
            {
            public:
                WorldUpdate() : Packet{OpcodeServer::OPCODE_SERVER_SNAPSHOT}
                {

                }
                void OnRead(Util::Buffer::Reader reader) override;
                void OnWrite() override;

                Snapshot snapShot;
            };
            
            class PlayerDisconnected final : public Packet
            {
            public:
                PlayerDisconnected() : Packet{OpcodeServer::OPCODE_SERVER_PLAYER_DISCONNECTED}
                {
                    
                }

                void OnRead(Util::Buffer::Reader reader) override;
                void OnWrite() override;

                Entity::ID playerId;
            };

            class PlayerInfo final : public Packet
            {
            public:
                PlayerInfo() : Packet{OpcodeServer::OPCODE_SERVER_PLAYER_INFO}
                {
                    
                }

                void OnRead(Util::Buffer::Reader reader) override;
                void OnWrite() override;

                Entity::PlayerState playerState;
                uint32_t lastCmd = 0;
            };
        }

        namespace Client
        {
            class Login final : public Packet
            {   
            public:
                Login() : Packet{OpcodeClient::OPCODE_CLIENT_LOGIN}
                {

                }
                void OnRead(Util::Buffer::Reader reader) override;
                void OnWrite() override;

                std::string playerUuid;
            };

            class Input final : public Packet
            {   
            public:

                struct Req
                {
                    uint32_t reqId;
                    Entity::PlayerInput playerInput;
                    float camYaw;
                    float camPitch;
                    float fov; // This is affected player's zoom level
                    float aspectRatio;
                    Util::Time::Seconds renderTime;
                };

                Input() : Packet{OpcodeClient::OPCODE_CLIENT_INPUT}
                {

                }
                void OnRead(Util::Buffer::Reader reader) override;
                void OnWrite() override;

                Req req;
            };
        }
    }
}