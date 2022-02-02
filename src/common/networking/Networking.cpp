#include <Networking.h>

using namespace Networking;

void Packet::Read()
{
    auto reader = buffer.GetReader();

    OnRead(reader);
}

void Packet::Write()
{
    buffer.Write(GetSize());
    buffer.Write(opCode);

    OnWrite();
}

template<>
std::unique_ptr<Packet> Networking::MakePacket<PacketType::Server>(uint16_t opCode)
{
    using namespace Networking::Packets::Server;

    std::unique_ptr<Packet> packet = nullptr;

    switch (opCode)
    {
    case OpcodeServer::WELCOME:
        packet = std::make_unique<Welcome>();
        break;

    case OpcodeServer::SNAPSHOT:
        packet = std::make_unique<Packets::Server::Snapshot>();
        break;
    
    default:
        break;
    }

    return packet;
}

void Batch::Write()
{
    buffer.Clear();

    for(auto& packet : packets)
    {
        packet->Write();
        buffer.Append(std::move(*packet->GetBuffer()));
    }
}

using namespace Networking::Packets;

void Server::Welcome::OnRead(Util::Buffer::Reader reader)
{
    playerId = reader.Read<uint8_t>();
    tickRate = reader.Read<double>();
}

void Server::Welcome::OnWrite()
{
    buffer.Write(playerId);
    buffer.Write(tickRate);
}

void Server::Snapshot::OnRead(Util::Buffer::Reader reader)
{
    lastCmd = reader.Read<uint32_t>();
    serverTick = reader.Read<uint32_t>();

    auto len = reader.Read<std::size_t>();
    for(auto i = 0; i < len; i++)
    {
        auto id = reader.Read<Entity::ID>();
        auto playerState = reader.Read<PlayerState>();
        players[id] = playerState;
    }
}

void Server::Snapshot::OnWrite()
{
    buffer.Write(lastCmd);
    buffer.Write(serverTick);

    buffer.Write(players.size());
    for(auto& [id, playerState] : players)
    {
        buffer.Write(id);
        buffer.Write(playerState);
    }
}