#include <Snapshot.h>

using namespace Networking;

Util::Buffer Snapshot::ToBuffer() const
{
    Util::Buffer buffer;

    // Server tick
    buffer.Write(serverTick);

    // Players' state
    buffer.Write<uint8_t>(players.size());
    for(auto& [id, state] : players)
    {
        buffer.Write(id);
        buffer.Write(state);
    }

    return buffer;
}

Snapshot Snapshot::FromBuffer(Util::Buffer::Reader& reader)
{    
    Snapshot s;

    // Server tick
    s.serverTick = reader.Read<uint32_t>();

    auto players = reader.Read<uint8_t>();
    for(auto i = 0; i < players; i++)
    {
        auto id = reader.Read<Entity::ID>();
        auto playerState = reader.Read<Entity::PlayerState>();
        s.players[id] = playerState;
    }

    return s;
}