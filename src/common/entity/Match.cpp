#include <Match.h>

using namespace BlockBuster;

const std::string BlockBuster::GameMode::typeStrings[BlockBuster::GameModeType::COUNT] = {"Free for All", "Team DeathMatch", "Domination", "Capture the Flag"};

std::unordered_map<GameModeType, bool> BlockBuster::GetSupportedGameModes(Game::Map::Map& map)
{
    std::unordered_map<GameModeType, bool> gameModes;

    auto goIndices = map.GetGameObjectIndices();
    gameModes[GameModeType::FREE_FOR_ALL] = !map.FindGameObjectByType(Entity::GameObject::RESPAWN).empty();
    gameModes[GameModeType::TEAM_DEATHMATCH] = gameModes[GameModeType::FREE_FOR_ALL];
    gameModes[GameModeType::DOMINATION] = gameModes[GameModeType::FREE_FOR_ALL] && !map.FindGameObjectByType(Entity::GameObject::DOMINATION_POINT).empty();
    gameModes[GameModeType::CAPTURE_THE_FLAG] = gameModes[GameModeType::FREE_FOR_ALL] && !map.FindGameObjectByType(Entity::GameObject::FLAG_SPAWN_A).empty()
        && !map.FindGameObjectByType(Entity::GameObject::FLAG_SPAWN_B).empty();;

    return gameModes;
}

std::vector<std::string> BlockBuster::GetSupportedGameModesAsString(Game::Map::Map& map)
{
    std::vector<std::string> gameModesStr;
    auto gameModes = GetSupportedGameModes(map);

    for(auto [mode, supported] : gameModes)
    {
        if(supported)
            gameModesStr.push_back(GameMode::typeStrings[mode]);
    }

    return gameModesStr;
}