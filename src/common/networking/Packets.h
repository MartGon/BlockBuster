#pragma once


#include <memory>

#include <networking/Networking.h>
#include <entity/Player.h>
#include <entity/Match.h>
#include <entity/Event.h>

#include <Snapshot.h>

#include <enetw/ENetW.h>

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
        OPCODE_SERVER_MATCH_STATE,
        OPCODE_SERVER_SNAPSHOT,
        OPCODE_SERVER_PLAYER_JOINED,
        OPCODE_SERVER_PLAYER_DISCONNECTED,
        OPCODE_SERVER_PLAYER_INPUT_ACK,
        OPCODE_SERVER_PLAYER_TAKE_DMG,
        OPCODE_SERVER_PLAYER_HIT_CONFIRM,
        OPCODE_SERVER_PLAYER_DIED,
        OPCODE_SERVER_PLAYER_RESPAWN,
        OPCODE_SERVER_PLAYER_WARPED,
        OPCODE_SERVER_SCOREBOARD_REPORT,
        OPCODE_SERVER_GAME_EVENT,
        OPCODE_SERVER_GAMEOBJECT_STATE,
        OPCODE_SERVER_PLAYER_GAMEOBJECT_INTERACT,
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

        void OnRead(Util::Buffer::Reader& reader) override
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

        uint8_t GetFlags() const override
        {
            uint8_t flags = 0;
            for(auto& packet : packets)
                flags |= packet->GetFlags();

            return flags;
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
                Welcome() : Packet{OpcodeServer::OPCODE_SERVER_WELCOME, ENET_PACKET_FLAG_RELIABLE}
                {

                }
                void OnRead(Util::Buffer::Reader& reader) override;
                void OnWrite() override;

                Entity::ID playerId;
                Entity::ID teamId;
                double tickRate;
                uint8_t startingPlayers;
                BlockBuster::GameMode::Type mode;
            };

            class MatchState final : public Packet
            {
            public:
                MatchState() : Packet{OpcodeServer::OPCODE_SERVER_MATCH_STATE, ENET_PACKET_FLAG_RELIABLE}
                {

                }
                void OnRead(Util::Buffer::Reader& reader) override;
                void OnWrite() override;

                BlockBuster::Match::State state;
            };

            class WorldUpdate final : public Packet
            {
            public:
                WorldUpdate() : Packet{OpcodeServer::OPCODE_SERVER_SNAPSHOT}
                {

                }
                void OnRead(Util::Buffer::Reader& reader) override;
                void OnWrite() override;

                Snapshot snapShot;
            };

            class PlayerJoined final : public Packet
            {
            public:
                PlayerJoined() : Packet{OpcodeServer::OPCODE_SERVER_PLAYER_JOINED, ENET_PACKET_FLAG_RELIABLE}
                {
                    
                }

                void OnRead(Util::Buffer::Reader& reader) override;
                void OnWrite() override;

                Entity::ID playerId;
                Entity::ID teamId;
                PlayerSnapshot playerSnapshot;
            };
            
            class PlayerDisconnected final : public Packet
            {
            public:
                PlayerDisconnected() : Packet{OpcodeServer::OPCODE_SERVER_PLAYER_DISCONNECTED, ENET_PACKET_FLAG_RELIABLE}
                {
                    
                }

                void OnRead(Util::Buffer::Reader& reader) override;
                void OnWrite() override;

                Entity::ID playerId;
            };

            class PlayerInputACK final : public Packet
            {
            public:
                PlayerInputACK() : Packet{OpcodeServer::OPCODE_SERVER_PLAYER_INPUT_ACK}
                {
                    
                }

                void OnRead(Util::Buffer::Reader& reader) override;
                void OnWrite() override;

                Entity::PlayerState playerState;
                uint32_t lastCmd = 0;
            };

            class PlayerTakeDmg final : public Packet
            {
            public:
                PlayerTakeDmg() : Packet{OpcodeServer::OPCODE_SERVER_PLAYER_TAKE_DMG, ENET_PACKET_FLAG_RELIABLE}
                {
                    
                }

                void OnRead(Util::Buffer::Reader& reader) override;
                void OnWrite() override;

                glm::vec3 origin;
                Entity::Player::HealthState healthState;
            };

            class PlayerHitConfirm final : public Packet
            {
            public:
                PlayerHitConfirm() : Packet{OpcodeServer::OPCODE_SERVER_PLAYER_HIT_CONFIRM, ENET_PACKET_FLAG_RELIABLE}
                {
                    
                }

                void OnRead(Util::Buffer::Reader& reader) override;
                void OnWrite() override;

                Entity::ID victimId;
            };

            class PlayerDied final : public Packet
            {
            public:
                PlayerDied() : Packet{OpcodeServer::OPCODE_SERVER_PLAYER_DIED, ENET_PACKET_FLAG_RELIABLE}
                {
                    
                }

                void OnRead(Util::Buffer::Reader& reader) override;
                void OnWrite() override;

                Entity::ID killerId;
                Entity::ID victimId;
                Util::Time::Seconds respawnTime;
            };

            class PlayerRespawn final : public Packet
            {
            public:
                PlayerRespawn() : Packet{OpcodeServer::OPCODE_SERVER_PLAYER_RESPAWN, ENET_PACKET_FLAG_RELIABLE}
                {
                    
                }

                void OnRead(Util::Buffer::Reader& reader) override;
                void OnWrite() override;

                Entity::ID playerId;
                Entity::PlayerState playerState;
                Entity::WeaponTypeID weapons[Entity::Player::MAX_WEAPONS];
            };

            class PlayerWarped final : public Packet
            {
            public:
                PlayerWarped() : Packet{OpcodeServer::OPCODE_SERVER_PLAYER_WARPED, ENET_PACKET_FLAG_RELIABLE}
                {
                    
                }

                void OnRead(Util::Buffer::Reader& reader) override;
                void OnWrite() override;

                Entity::ID playerId;
                Entity::PlayerState playerState;
            };

            class ScoreboardReport final : public Packet
            {
            public:
                ScoreboardReport() : Packet{OpcodeServer::OPCODE_SERVER_SCOREBOARD_REPORT, ENET_PACKET_FLAG_RELIABLE}
                {
                    
                }

                void OnRead(Util::Buffer::Reader& reader) override;
                void OnWrite() override;

                BlockBuster::Scoreboard scoreboard;
            };

            class GameEvent final : public Packet
            {
            public:
                GameEvent() : Packet{OpcodeServer::OPCODE_SERVER_GAME_EVENT, ENET_PACKET_FLAG_RELIABLE}
                {
                    
                }

                void OnRead(Util::Buffer::Reader& reader) override;
                void OnWrite() override;

                BlockBuster::Event event;
            };

            class GameObjectState final : public Packet
            {
            public:
                GameObjectState() : Packet{OpcodeServer::OPCODE_SERVER_GAMEOBJECT_STATE, ENET_PACKET_FLAG_RELIABLE}
                {
                    
                }

                void OnRead(Util::Buffer::Reader& reader) override;
                void OnWrite() override;

                glm::ivec3 goPos;
                Entity::GameObject::State state;
            };

            class PlayerGameObjectInteract final : public Packet
            {
            public:
                PlayerGameObjectInteract() : Packet{OpcodeServer::OPCODE_SERVER_PLAYER_GAMEOBJECT_INTERACT, ENET_PACKET_FLAG_RELIABLE}
                {
                    
                }

                void OnRead(Util::Buffer::Reader& reader) override;
                void OnWrite() override;

                glm::ivec3 goPos;
            };
        }

        namespace Client
        {
            class Login final : public Packet
            {   
            public:
                Login() : Packet{OpcodeClient::OPCODE_CLIENT_LOGIN, ENET_PACKET_FLAG_RELIABLE}
                {

                }
                void OnRead(Util::Buffer::Reader& reader) override;
                void OnWrite() override;

                std::string playerUuid;
                std::string playerName;
            };

            class Input final : public Packet
            {   
            public:

                struct Req
                {
                    uint32_t reqId ;
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
                void OnRead(Util::Buffer::Reader& reader) override;
                void OnWrite() override;

                Req req;
            };
        }
    }
}