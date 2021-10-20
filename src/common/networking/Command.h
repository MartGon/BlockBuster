#pragma once

#include <glm/glm.hpp>

namespace Networking
{
    struct Command
    {
        enum Type
        {
            CLIENT_CONFIG,
            PLAYER_POS_UPDATE,
            PLAYER_DISCONNECTED,

            PLAYER_MOVEMENT,
            ACK_COMMAND
        };

        struct Server
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
        };

        union Payload
        {
            Server::ClientConfig config;
            Server::PlayerUpdate playerUpdate;
            Server::PlayerDisconnected playerDisconnect;
            Server::AckCommand ackCommand;

            User::PlayerMovement playerMovement;
        };

        Header header;
        Payload data;
    };
}