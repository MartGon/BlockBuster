#include <Snapshot.h>

#include <math/Interpolation.h>

using namespace Networking;

PlayerSnapshot PlayerSnapshot::FromPlayerState(Entity::PlayerState playerState)
{
    PlayerSnapshot ps;
    ps.transform = playerState.transform;
    ps.wepState = playerState.weaponState.state;
    return ps;
}

Entity::PlayerState PlayerSnapshot::ToPlayerState(Entity::PlayerState ps)
{
    ps.transform = transform;
    ps.weaponState.state = wepState;

    return ps;
}

PlayerSnapshot PlayerSnapshot::Interpolate(PlayerSnapshot a, PlayerSnapshot b, float alpha)
{
    auto res = a;

    res.transform = Entity::Interpolate(a.transform, b.transform, alpha);
    res.wepState = a.wepState;

    return res;
}

Util::Buffer Snapshot::ToBuffer() const
{
    Util::Buffer buffer;

    // Server tick
    buffer.Write(serverTick);

    // Players' snapshot
    buffer.Write<uint8_t>(players.size());
    for(auto& [id, ps] : players)
    {
        buffer.Write(id);
        buffer.Write(ps);
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
        auto playerSnapshot = reader.Read<PlayerSnapshot>();
        s.players[id] = playerSnapshot;
    }

    return s;
}