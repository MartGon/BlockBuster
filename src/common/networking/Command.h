#pragma once

#include <glm/glm.hpp>

#include <util/Buffer.h>

#include <networking/Snapshot.h>

namespace Networking
{
    struct Command
    {
        enum Type
        {
            // Server
            CLIENT_CONFIG,
            SERVER_SNAPSHOT,
            PLAYER_DISCONNECTED,
            ACK_COMMAND,

            // Client
            PLAYER_MOVEMENT,
        };

        struct Server
        {
            struct ClientConfig
            {
                uint8_t playerId;
                double sampleRate;
            };

            struct Snapshot
            {
            };

            struct PlayerDisconnected
            {
                uint8_t playerId;
            };

            struct AckCommand
            {
                uint32_t commandId;
            };
        };

        struct User
        {
            struct PlayerMovement
            {
                glm::vec3 moveDir;
            };
        };

        struct Header
        {
            Command::Type type;
            uint32_t tick;

            Util::Buffer ToBuffer() const;
        };

        union Payload
        {
            Server::ClientConfig config;
            Server::Snapshot snapshot;
            Server::PlayerDisconnected playerDisconnect;
            Server::AckCommand ackCommand;

            User::PlayerMovement playerMovement;
        };

        Header header;
        Payload data;
    };
}