#include <Snapshot.h>

#include <math/Interpolation.h>

using namespace Networking;

PlayerSnapshot PlayerSnapshot::FromPlayerState(Entity::PlayerState playerState)
{
    PlayerSnapshot ps;
    ps.pos = playerState.pos;
    ps.rot = playerState.rot;
    ps.wepState = playerState.weaponState.state;
    return ps;
}

Entity::PlayerState PlayerSnapshot::ToPlayerState(Entity::PlayerState ps)
{
    ps.pos = pos;
    ps.rot = rot;
    ps.weaponState.state = wepState;

    return ps;
}

PlayerSnapshot PlayerSnapshot::Interpolate(PlayerSnapshot a, PlayerSnapshot b, float alpha)
{
    auto res = a;

    auto pos1 = a.pos;
    auto pos2 = b.pos;
    res.pos = pos1 * alpha + pos2 * (1 - alpha);

    auto pitch1 = a.rot.x;
    auto pitch2 = b.rot.x;
    res.rot.x = Math::InterpolateDeg(pitch1, pitch2, alpha);

    auto yaw1 = a.rot.y;
    auto yaw2 = b.rot.y;
    res.rot.y = Math::InterpolateDeg(yaw1, yaw2, alpha);

    a.wepState = b.wepState;

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