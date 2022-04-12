#pragma once

#include <unordered_map>

#include <entity/Player.h>
#include <entity/Weapon.h>
#include <entity/Projectile.h>

#include <util/Buffer.h>

namespace Networking
{
    struct PlayerSnapshot
    {
        Entity::PlayerState::Transform transform;
        Entity::Weapon::State wepState;
        Entity::WeaponTypeID weaponId;

        static PlayerSnapshot FromPlayerState(Entity::PlayerState playerState);
        Entity::PlayerState ToPlayerState(Entity::PlayerState playerState);
        static PlayerSnapshot Interpolate(PlayerSnapshot a, PlayerSnapshot b, float alpha);
    };


    struct Snapshot
    {
        uint32_t serverTick = 0;
        std::unordered_map<Entity::ID, PlayerSnapshot> players;
        std::unordered_map<Entity::ID, Entity::Projectile::State> projectiles;

        Util::Buffer ToBuffer() const;
        static Snapshot FromBuffer(Util::Buffer::Reader& reader);
    };
}