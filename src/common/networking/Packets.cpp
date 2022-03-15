
#include <Packets.h>

using namespace Networking;

template<>
std::unique_ptr<Packet> Networking::MakePacket<PacketType::Server>(uint16_t opCode)
{
    using namespace Networking::Packets::Server;

    std::unique_ptr<Packet> packet = nullptr;

    switch (opCode)
    {
    case OpcodeServer::OPCODE_SERVER_BATCH:
        packet = std::make_unique<Networking::Batch<Networking::PacketType::Server>>();
        break;

    case OpcodeServer::OPCODE_SERVER_WELCOME:
        packet = std::make_unique<Welcome>();
        break;

    case OpcodeServer::OPCODE_SERVER_SNAPSHOT:
        packet = std::make_unique<Packets::Server::WorldUpdate>();
        break;
    
    default:
        break;
    }

    return packet;
}

template<>
std::unique_ptr<Packet> Networking::MakePacket<PacketType::Client>(uint16_t opCode)
{
    using namespace Networking::Packets::Client;

    std::unique_ptr<Packet> packet = nullptr;

    switch (opCode)
    {
    case OpcodeClient::OPCODE_CLIENT_BATCH:
        packet = std::make_unique<Networking::Batch<Networking::PacketType::Client>>();
        break;

    case OpcodeClient::OPCODE_CLIENT_INPUT:
        packet = std::make_unique<Input>();
        break;
    
    default:
        break;
    }

    return packet;
}

template<>
Batch<Networking::PacketType::Client>::Batch() : Packet{OPCODE_CLIENT_BATCH}
{

}

template<>
Batch<Networking::PacketType::Server>::Batch() : Packet{OPCODE_SERVER_BATCH}
{
    
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
        snapShot.players.insert({id, playerState});
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
    req = reader.Read<Req>();
}

void Input::OnWrite()
{
    buffer.Write(req);
}