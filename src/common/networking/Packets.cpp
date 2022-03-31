
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

    case OpcodeServer::OPCODE_SERVER_MATCH_STATE:
        packet = std::make_unique<MatchState>();
        break;

    case OpcodeServer::OPCODE_SERVER_SNAPSHOT:
        packet = std::make_unique<Packets::Server::WorldUpdate>();
        break;
    
    case OpcodeServer::OPCODE_SERVER_PLAYER_JOINED:
        packet = std::make_unique<PlayerJoined>();
        break;

    case OpcodeServer::OPCODE_SERVER_PLAYER_DISCONNECTED:
        packet = std::make_unique<PlayerDisconnected>();
        break;

    case OpcodeServer::OPCODE_SERVER_PLAYER_INPUT_ACK:
        packet = std::make_unique<PlayerInputACK>();
        break;

    case OpcodeServer::OPCODE_SERVER_PLAYER_TAKE_DMG:
        packet = std::make_unique<PlayerTakeDmg>();
        break;

    case OpcodeServer::OPCODE_SERVER_PLAYER_HIT_CONFIRM:
        packet = std::make_unique<PlayerHitConfirm>();
        break;

    case OpcodeServer::OPCODE_SERVER_PLAYER_DIED:
        packet = std::make_unique<PlayerDied>();
        break;

    case OpcodeServer::OPCODE_SERVER_PLAYER_RESPAWN:
        packet = std::make_unique<PlayerRespawn>();
        break;

    case OpcodeServer::OPCODE_SERVER_SCOREBOARD_REPORT:
        packet = std::make_unique<ScoreboardReport>();
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

    case OpcodeClient::OPCODE_CLIENT_LOGIN:
        packet = std::make_unique<Login>();
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

void Welcome::OnRead(Util::Buffer::Reader& reader)
{
    playerId = reader.Read<Entity::ID>();
    teamId = reader.Read<Entity::ID>();
    tickRate = reader.Read<double>();
    mode = reader.Read<BlockBuster::GameMode::Type>();
    buffer.Write(mode);
}

void Welcome::OnWrite()
{
    buffer.Write(playerId);
    buffer.Write(teamId);
    buffer.Write(tickRate);
    buffer.Write(mode);
}

void MatchState::OnRead(Util::Buffer::Reader& reader)
{
    state = reader.Read<BlockBuster::Match::State>();
}

void MatchState::OnWrite()
{
    buffer.Write(state);
}

void WorldUpdate::OnRead(Util::Buffer::Reader& reader)
{
    snapShot.serverTick = reader.Read<uint32_t>();

    auto len = reader.Read<std::uint8_t>();
    for(auto i = 0; i < len; i++)
    {
        auto id = reader.Read<Entity::ID>();
        auto playerState = reader.Read<PlayerSnapshot>();
        snapShot.players.insert({id, playerState});
    }
}

void WorldUpdate::OnWrite()
{
    buffer.Append(this->snapShot.ToBuffer());
}
 
void PlayerJoined::OnRead(Util::Buffer::Reader& reader)
{
    playerId = reader.Read<Entity::ID>();
    teamId = reader.Read<Entity::ID>();
    playerSnapshot = reader.Read<PlayerSnapshot>();
}

void PlayerJoined::OnWrite()
{
    buffer.Write(playerId);
    buffer.Write(teamId);
    buffer.Write(playerSnapshot);
}

void PlayerDisconnected::OnRead(Util::Buffer::Reader& reader)
{
    playerId = reader.Read<Entity::ID>();
}

void PlayerDisconnected::OnWrite()
{
    buffer.Write(playerId);
}

void PlayerInputACK::OnRead(Util::Buffer::Reader& reader)
{
    lastCmd = reader.Read<uint32_t>();
    playerState = reader.Read<Entity::PlayerState>();
}

void PlayerInputACK::OnWrite()
{
    buffer.Write(lastCmd);
    buffer.Write(playerState);
}

void PlayerTakeDmg::OnRead(Util::Buffer::Reader& reader)
{
    origin = reader.Read<glm::vec3>();
    healthState = reader.Read<Entity::Player::HealthState>();
}

void PlayerTakeDmg::OnWrite()
{
    buffer.Write(origin);
    buffer.Write(healthState);
}

void PlayerHitConfirm::OnRead(Util::Buffer::Reader& reader)
{
    victimId = reader.Read<Entity::ID>();
}

void PlayerHitConfirm::OnWrite()
{
    buffer.Write(victimId);
}

void PlayerDied::OnRead(Util::Buffer::Reader& reader)
{
    killerId = reader.Read<Entity::ID>();
    victimId = reader.Read<Entity::ID>();
    respawnTime = reader.Read<Util::Time::Seconds>();
}

void PlayerDied::OnWrite()
{
    buffer.Write(killerId);
    buffer.Write(victimId);
    buffer.Write(respawnTime);
}

void PlayerRespawn::OnRead(Util::Buffer::Reader& reader)
{
    playerId = reader.Read<Entity::ID>();
    playerState = reader.Read<Entity::PlayerState>();
}

void PlayerRespawn::OnWrite()
{
    buffer.Write(playerId);
    buffer.Write(playerState);
}

void ScoreboardReport::OnRead(Util::Buffer::Reader& reader)
{
    scoreboard = BlockBuster::Scoreboard::FromBuffer(reader);
}

void ScoreboardReport::OnWrite()
{
    buffer.Append(scoreboard.ToBuffer());
}

using namespace Networking::Packets::Client;

void Login::OnRead(Util::Buffer::Reader& reader)
{
    playerUuid = reader.Read<std::string>();
    playerName = reader.Read<std::string>();
}

void Login::OnWrite()
{
    buffer.Write(playerUuid);
    buffer.Write(playerName);
}

void Input::OnRead(Util::Buffer::Reader& reader)
{
    req = reader.Read<Req>();
}

void Input::OnWrite()
{
    buffer.Write(req);
}