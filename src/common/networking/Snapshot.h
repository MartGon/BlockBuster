#pragma once

#include <unordered_map>

#include <entity/Player.h>

#include <util/Buffer.h>

namespace Networking
{
    struct Snapshot
    {
        uint32_t serverTick = 0;
        std::unordered_map<Entity::ID, Entity::PlayerState> players;

        Util::Buffer ToBuffer() const;
        static Snapshot FromBuffer(Util::Buffer::Reader& reader);
    };
}