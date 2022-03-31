#include <GameMode.h>

#include <util/Container.h>

using namespace BlockBuster;

const std::string BlockBuster::GameMode::typeStrings[GameMode::Type::COUNT] = {"Free for All", "Team DeathMatch", "Domination", "Capture the Flag"};
const std::unordered_map<std::string, GameMode::Type> GameMode::stringTypes = {
    {"Free for All", GameMode::Type::FREE_FOR_ALL}, 
    {"Team DeathMatch", GameMode::Type::TEAM_DEATHMATCH}, 
    {"Domination", GameMode::Type::DOMINATION},
    {"Capture the Flag", GameMode::Type::CAPTURE_THE_FLAG}
};

// Player Score

Util::Buffer PlayerScore::ToBuffer()
{
    Util::Buffer buffer;
    buffer.Write(playerId);
    buffer.Write(name);
    buffer.Write(kills);
    buffer.Write(score);
    buffer.Write(deaths);

    return std::move(buffer);
}

PlayerScore PlayerScore::FromBuffer(Util::Buffer::Reader& reader)
{
    PlayerScore ps;
    ps.playerId = reader.Read<decltype(ps.playerId)>();
    ps.name = reader.Read<decltype(ps.name)>();
    ps.kills = reader.Read<decltype(ps.kills)>();
    ps.score = reader.Read<decltype(ps.score)>();
    ps.deaths = reader.Read<decltype(ps.deaths)>();

    return ps;
}

// Scoreboard

void Scoreboard::AddPlayer(Entity::ID playerId, std::string name)
{
    PlayerScore ps{playerId, name, 0, 0};
    SetPlayerScore(ps);
}

void Scoreboard::SetPlayerScore(PlayerScore ps)
{
    playersScore[ps.playerId] = ps;
    hasChanged = true;
}

void Scoreboard::RemovePlayer(Entity::ID playerId)
{
    playersScore.erase(playerId);
    hasChanged = true;
}

std::vector<Entity::ID> Scoreboard::GetPlayerIDs()
{
    std::vector<Entity::ID> players;
    for(auto& [id, score] : playersScore)
        players.push_back(id);

    return players;
}

std::optional<PlayerScore> Scoreboard::GetPlayerScore(Entity::ID playerId)
{
    std::optional<PlayerScore> ps;
    if(Util::Map::Contains(playersScore, playerId))
        ps = playersScore[playerId];
    
    return ps;
}

void Scoreboard::AddTeam(Entity::ID teamId)
{
    TeamScore ts{teamId, 0};
    SetTeamScore(ts);
}

void Scoreboard::SetTeamScore(TeamScore score)
{
    teamsScore[score.teamId] = score;
    hasChanged = true;
}

void Scoreboard::RemoveTeam(Entity::ID teamId)
{
    teamsScore.erase(teamId);
    hasChanged = true;
}

std::vector<Entity::ID> Scoreboard::GetTeamIDs()
{
    std::vector<Entity::ID> teams;
    for(auto& [id, score] : teamsScore)
        teams.push_back(id);

    return teams;
}

std::optional<TeamScore> Scoreboard::GetTeamScore(Entity::ID teamId)
{
    std::optional<TeamScore> ts;
    if(Util::Map::Contains(teamsScore, teamId))
        ts = teamsScore[teamId];
    
    return ts;
}

Util::Buffer Scoreboard::ToBuffer()
{
    Util::Buffer buffer;

    buffer.Write(playersScore.size());
    for(auto& [id, score] : playersScore)
        buffer.Append(score.ToBuffer());

    buffer.Write(teamsScore.size());
    for(auto& [id, score] : teamsScore)
        buffer.Write(score);

    return std::move(buffer);
}

Scoreboard Scoreboard::FromBuffer(Util::Buffer::Reader& reader)
{
    Scoreboard scoreboard;

    auto players = reader.Read<std::size_t>();
    for(int i = 0; i < players; i++)
    {
        auto playerScore = PlayerScore::FromBuffer(reader);
        scoreboard.playersScore[playerScore.playerId] = playerScore;
    }

    auto teams = reader.Read<std::size_t>();
    for(int i = 0; i < teams; i++)
    {
        auto teamScore = reader.Read<TeamScore>();
        scoreboard.teamsScore[teamScore.teamId] = teamScore;
    }

    return scoreboard;
}

// Gamemode

// Other

std::unique_ptr<GameMode> BlockBuster::CreateGameMode(GameMode::Type type)
{
    std::unique_ptr<GameMode> gameMode;

    switch (type)
    {
    case GameMode::Type::NULL_MODE:
        gameMode = nullptr;
        break;

    case GameMode::Type::FREE_FOR_ALL:
        gameMode = std::make_unique<FreeForAll>();
        break;

    case GameMode::Type::TEAM_DEATHMATCH:
        assertm(false, "This mode has not been implemented yet");
        //gameMode = std::make_unique<FreeForAll>();
        break;

    case GameMode::Type::DOMINATION:
        assertm(false, "This mode has not been implemented yet");
        //gameMode = std::make_unique<FreeForAll>();
        break;

    case GameMode::Type::CAPTURE_THE_FLAG:
        assertm(false, "This mode has not been implemented yet");
        //gameMode = std::make_unique<FreeForAll>();
        break;
    
    default:
        break;
    }

    return gameMode;
}

std::unordered_map<GameMode::Type, bool> BlockBuster::GetSupportedGameModes(Game::Map::Map& map)
{
    std::unordered_map<GameMode::Type, bool> gameModes;

    auto goIndices = map.GetGameObjectIndices();
    gameModes[GameMode::Type::FREE_FOR_ALL] = !map.FindGameObjectByType(Entity::GameObject::RESPAWN).empty();
    gameModes[GameMode::Type::TEAM_DEATHMATCH] = gameModes[GameMode::Type::FREE_FOR_ALL];
    gameModes[GameMode::Type::DOMINATION] = gameModes[GameMode::Type::FREE_FOR_ALL] && !map.FindGameObjectByType(Entity::GameObject::DOMINATION_POINT).empty();
    gameModes[GameMode::Type::CAPTURE_THE_FLAG] = gameModes[GameMode::Type::FREE_FOR_ALL] && !map.FindGameObjectByType(Entity::GameObject::FLAG_SPAWN_A).empty()
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

// GameMode - FreeForAll

void FreeForAll::OnPlayerDeath(Entity::ID killer, Entity::ID victim, Entity::ID killerTeamId)
{
    if(auto score = scoreBoard.GetPlayerScore(killer))
    {
        score->kills++;
        score->score++;
        scoreBoard.SetPlayerScore(score.value());
    }

    if(auto score = scoreBoard.GetPlayerScore(victim))
    {
        score->deaths++;
        scoreBoard.SetPlayerScore(score.value());
    }
}

bool FreeForAll::IsGameOver()
{
    auto players = scoreBoard.GetPlayerIDs();
    for(auto& pid : players)
        if(scoreBoard.GetPlayerScore(pid)->kills >= MAX_KILLS)
            return true;

    return false;
}

// GameMode - Team DeathMatch

void TeamDeathMatch::OnPlayerDeath(Entity::ID killer, Entity::ID victim, Entity::ID killerTeamId)
{
    if(auto score = scoreBoard.GetPlayerScore(killer))
    {
        score->kills++;
        score->score++;
        scoreBoard.SetPlayerScore(score.value());
    }

    if(auto teamScore = scoreBoard.GetTeamScore(killerTeamId))
    {
        teamScore->score++;
        scoreBoard.SetTeamScore(teamScore.value());
    }

    if(auto score = scoreBoard.GetPlayerScore(victim))
    {
        score->deaths++;
        scoreBoard.SetPlayerScore(score.value());
    }
}

bool TeamDeathMatch::IsGameOver()
{
    auto teams = scoreBoard.GetTeamIDs();
    for(auto& tid : teams)
        if(scoreBoard.GetTeamScore(tid)->score >= MAX_KILLS)
            return true;

    return false;
}