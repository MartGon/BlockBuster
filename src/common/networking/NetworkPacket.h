#pragma once

#include <glm/glm.hpp>

namespace Networking
{
    namespace Payload
    {
        struct ClientConfig
        {
            uint8_t playerId;
            double sampleRate;
        };

        struct PlayerUpdate
        {
            uint8_t playerId;
            glm::vec3 pos;
        };

        struct PlayerMovement
        {
            uint8_t playerId; // TODO: This shouldn't be needed. Keep a table in server that maps peerId to playerId
            glm::vec3 moveDir;
        };

        union Data
        {
            ClientConfig config;
            PlayerUpdate playerUpdate;
            PlayerMovement playerMovement;
        };
    }

    struct Packet
    {
        enum Type
        {
            CLIENT_CONFIG,
            PLAYER_UPDATE,
            PLAYER_MOVEMENT
        };

        struct Header
        {
            Type type;
            uint32_t tick;
        };

        Header header;
        Payload::Data data;
    };
}