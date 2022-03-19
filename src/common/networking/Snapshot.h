#pragma once

#include <unordered_map>

#include <entity/Player.h>
#include <entity/Weapon.h>

#include <util/Buffer.h>

namespace Networking
{
    struct PlayerSnapshot
    {
        glm::vec3 pos;
        glm::vec2 rot;
        Entity::Weapon::State wepState;

        static PlayerSnapshot FromPlayerState(Entity::PlayerState playerState);
        Entity::PlayerState ToPlayerState(Entity::PlayerState playerState);
        static PlayerSnapshot Interpolate(PlayerSnapshot a, PlayerSnapshot b, float alpha);
    };


    struct Snapshot
    {
        uint32_t serverTick = 0;
        std::unordered_map<Entity::ID, PlayerSnapshot> players;

        Util::Buffer ToBuffer() const;
        static Snapshot FromBuffer(Util::Buffer::Reader& reader);
    };
}