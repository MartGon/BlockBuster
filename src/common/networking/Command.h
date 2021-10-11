#pragma once

#include <glm/glm.hpp>

namespace Networking
{
    namespace Command
    {
        enum Type
        {
            CLIENT_CONFIG,
            PLAYER_UPDATE,

            PLAYER_MOVEMENT
        };

        namespace Server
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
        }

        namespace User
        {
            struct PlayerMovement
            {
                uint8_t playerId; // TODO: This shouldn't be needed. Keep a table in server that maps peerId to playerId
                glm::vec3 moveDir;
            };
        }
    }

    struct Packet
    {
        struct Header
        {
            Command::Type type;
            uint32_t tick;
        };

        union Payload
        {
            Command::Server::ClientConfig config;
            Command::Server::PlayerUpdate playerUpdate;

            Command::User::PlayerMovement playerMovement;
        };

        Header header;
        Payload data;
    };
}