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
            ACK_COMMAND, // Deprecated. Snapshot carry acks.

            // Client
            PLAYER_MOVEMENT,
            PLAYER_MOVEMENT_BATCH,
            PLAYER_SHOT,
        };

        struct Server
        {
            struct ClientConfig
            {
                uint8_t playerId;
                double sampleRate;
            };

            struct Update
            {
                uint32_t lastCmd;
                uint32_t snapshotDataSize;
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
            struct PlayerMovementBatch
            {
                uint32_t amount;
            };

            struct PlayerMovement
            {
                uint32_t reqId;
                glm::vec3 moveDir;
            };

            struct PlayerShot
            {
                glm::vec3 origin;
                glm::vec3 dir;
                double clientTime;
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
            Server::Update snapshot;
            Server::PlayerDisconnected playerDisconnect;
            Server::AckCommand ackCommand;

            User::PlayerMovement playerMovement;
            User::PlayerMovementBatch playerMovementBatch;
            User::PlayerShot playerShot;
        };

        Header header;
        Payload data;
    };
}