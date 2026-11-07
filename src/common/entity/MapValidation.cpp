#include <entity/MapValidation.h>


#include <map>
#include <algorithm>

using namespace BlockBuster;
using GameObject = Entity::GameObject;

bool MapValidation::HasErrors() const
{
    for(auto& issue : issues)
        if(issue.severity == MapIssue::ERROR)
            return true;

    return false;
}

static bool IsSolid(Game::Map::Map& map, glm::ivec3 pos)
{
    auto block = map.GetBlock(pos);
    return block && block->type != Game::BlockType::NONE;
}

static std::string PosToString(glm::ivec3 pos)
{
    return "(" + std::to_string(pos.x) + ", " + std::to_string(pos.y) + ", " + std::to_string(pos.z) + ")";
}

static void ValidateGameModes(Game::Map::Map& map, MapValidation& validation)
{
    auto supported = GetSupportedGameModes(map);
    bool hasRespawns = !map.FindGameObjectByType(GameObject::RESPAWN).empty();
    bool hasDomPoints = !map.FindGameObjectByType(GameObject::DOMINATION_POINT).empty();
    bool hasFlagA = !map.FindGameObjectByType(GameObject::FLAG_SPAWN_A).empty();
    bool hasFlagB = !map.FindGameObjectByType(GameObject::FLAG_SPAWN_B).empty();

    for(int i = GameMode::Type::FREE_FOR_ALL; i < GameMode::Type::COUNT; i++)
    {
        auto mode = static_cast<GameMode::Type>(i);
        GameModeSupport gms{mode, supported[mode], ""};
        if(!gms.supported)
        {
            if(!hasRespawns)
                gms.reason = "No respawn points";
            else if(mode == GameMode::DOMINATION && !hasDomPoints)
                gms.reason = "No domination points";
            else if(mode == GameMode::CAPTURE_THE_FLAG)
                gms.reason = "No flag spawns";
        }
        validation.modes.push_back(gms);
    }

    if(hasFlagA != hasFlagB)
    {
        std::string missing = hasFlagA ? "FLAG_SPAWN_B" : "FLAG_SPAWN_A";
        validation.issues.push_back({MapIssue::WARNING, "Capture the Flag has only one flag spawn. Missing " + missing, std::nullopt});
    }
}

static void ValidateRespawns(Game::Map::Map& map, MapValidation& validation)
{
    auto respawns = map.GetRespawnIndices();
    if(respawns.empty())
    {
        validation.issues.push_back({MapIssue::ERROR, "The map has no respawn points", std::nullopt});
        return;
    }

    // Team spawns. The respawn TeamId is a bit mask: 1 blue, 2 red, 3 both
    const std::pair<uint8_t, const char*> teams[] = {{1, "Blue"}, {2, "Red"}};
    for(auto [mask, name] : teams)
    {
        bool found = false;
        for(auto pos : respawns)
            if(auto respawn = map.GetRespawn(pos); respawn && (respawn->teamId & mask))
                found = true;

        if(!found)
            validation.issues.push_back({MapIssue::WARNING, std::string{name} + " team has no respawns. Team modes will spawn it anywhere", std::nullopt});
    }

    // Obstructed spawns. Players take the respawn block and the one above it
    for(auto pos : respawns)
    {
        auto above = pos + glm::ivec3{0, 1, 0};
        if(IsSolid(map, pos) || IsSolid(map, above))
            validation.issues.push_back({MapIssue::ERROR, "Respawn at " + PosToString(pos) + " is blocked. Players would spawn stuck", pos});
    }
}

static void ValidateTeleports(Game::Map::Map& map, MapValidation& validation)
{
    auto getChannel = [&map](glm::ivec3 pos)
    {
        auto go = map.GetGameObject(pos);
        return std::get<int>(go->properties["Channel ID"].value);
    };

    std::map<int, std::vector<glm::ivec3>> origins;
    std::map<int, std::vector<glm::ivec3>> dests;
    for(auto pos : map.FindGameObjectByType(GameObject::TELEPORT_ORIGIN))
        origins[getChannel(pos)].push_back(pos);
    for(auto pos : map.FindGameObjectByType(GameObject::TELEPORT_DEST))
        dests[getChannel(pos)].push_back(pos);

    for(auto& [channel, positions] : origins)
    {
        if(dests.find(channel) != dests.end())
            continue;

        for(auto pos : positions)
            validation.issues.push_back({MapIssue::ERROR, "Teleport at " + PosToString(pos) + " has no destination on channel " + std::to_string(channel), pos});
    }

    for(auto& [channel, positions] : dests)
    {
        if(origins.find(channel) != origins.end())
            continue;

        for(auto pos : positions)
            validation.issues.push_back({MapIssue::WARNING, "Teleport destination at " + PosToString(pos) + " has no teleport on channel " + std::to_string(channel), pos});
    }
}

MapValidation BlockBuster::ValidateMap(Game::Map::Map& map)
{
    MapValidation validation;

    ValidateGameModes(map, validation);
    ValidateRespawns(map, validation);
    ValidateTeleports(map, validation);

    // Errors first
    std::stable_sort(validation.issues.begin(), validation.issues.end(), [](auto& a, auto& b){
        return a.severity > b.severity;
    });

    return validation;
}
