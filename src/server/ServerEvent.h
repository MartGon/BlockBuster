#pragma once

#include <string>
#include <entity/Player.h>

#include <nlohmann/json.hpp>
#include <variant>

namespace BlockBuster::ServerEvent
{
    enum Type
    {
        PLAYER_LEFT,
        GAME_ENDED
    };

    struct PlayerLeft
    {
        std::string playerUuid;
    };

    struct GameEnded
    {

    };

    struct Notification
    {
        Type eventType;
        std::variant<PlayerLeft, GameEnded> event;

        nlohmann::json ToJson();
    };
}