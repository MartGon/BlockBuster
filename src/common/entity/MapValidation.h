#pragma once

#include <entity/Map.h>
#include <entity/GameMode.h>

#include <optional>
#include <string>
#include <vector>

namespace BlockBuster
{
    // Problems that make a map unplayable or annoying to play. Used by the editor's Validate tab
    struct MapIssue
    {
        enum Severity
        {
            WARNING,
            ERROR
        };

        Severity severity;
        std::string msg;
        std::optional<glm::ivec3> pos; // Block position the issue refers to, if any
    };

    struct GameModeSupport
    {
        GameMode::Type mode;
        bool supported;
        std::string reason; // Why it's not supported. Empty when supported
    };

    struct MapValidation
    {
        std::vector<GameModeSupport> modes; // In GameMode::Type order
        std::vector<MapIssue> issues;

        bool HasErrors() const;
    };

    MapValidation ValidateMap(Game::Map::Map& map);
}
