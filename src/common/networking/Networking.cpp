#include <Networking.h>

using namespace Networking;

void Packet::Read()
{
    auto reader = buffer.GetReader();

    OnRead(reader);
}

void Packet::Write()
{
    buffer.Clear();

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
        packet = std::make_unique<Packets::Server::WorldUpdate>();
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

using namespace Networking::Packets::Server;

void Welcome::OnRead(Util::Buffer::Reader reader)
{
    playerId = reader.Read<uint8_t>();
    tickRate = reader.Read<double>();
}

void Welcome::OnWrite()
{
    buffer.Write(playerId);
    buffer.Write(tickRate);
}

void WorldUpdate::OnRead(Util::Buffer::Reader reader)
{
    lastCmd = reader.Read<uint32_t>();
    snapShot.serverTick = reader.Read<uint32_t>();

    auto len = reader.Read<std::uint8_t>();
    for(auto i = 0; i < len; i++)
    {
        auto id = reader.Read<Entity::ID>();
        auto playerState = reader.Read<Entity::PlayerState>();
        snapShot.players[id] = playerState;
    }
}

void WorldUpdate::OnWrite()
{
    buffer.Write(lastCmd);
    buffer.Append(this->snapShot.ToBuffer());
}

using namespace Networking::Packets::Client;

void Input::OnRead(Util::Buffer::Reader reader)
{
    playerInput = reader.Read<Entity::PlayerInput>();
}

void Input::OnWrite()
{
    buffer.Write(playerInput);
}