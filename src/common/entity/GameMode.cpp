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
    buffer.Write(teamId);
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
    ps.teamId = reader.Read<decltype(ps.teamId)>();
    ps.name = reader.Read<decltype(ps.name)>();
    ps.kills = reader.Read<decltype(ps.kills)>();
    ps.score = reader.Read<decltype(ps.score)>();
    ps.deaths = reader.Read<decltype(ps.deaths)>();

    return ps;
}

// Scoreboard

void Scoreboard::AddPlayer(Entity::ID playerId, Entity::ID teamId, std::string name)
{
    PlayerScore ps{playerId, teamId, name, 0, 0};
    if(!Util::Map::Contains(teamsScore, teamId))
        SetTeamScore(TeamScore{teamId, 0});
    SetPlayerScore(ps);
}

void Scoreboard::SetPlayerScore(PlayerScore ps)
{
    playersScore[ps.playerId] = ps;
    hasChanged = true;
}

void Scoreboard::RemovePlayer(Entity::ID playerId)
{
    if(auto ps = GetPlayerScore(playerId))
    {
        playersScore.erase(playerId);
        if(GetTeamPlayers(ps->teamId).empty())
            RemoveTeam(ps->teamId);

        hasChanged = true;
    }
}

std::vector<Entity::ID> Scoreboard::GetPlayerIDs()
{
    std::vector<Entity::ID> players;
    for(auto& [id, score] : playersScore)
        players.push_back(id);

    return players;
}

std::vector<PlayerScore> Scoreboard::GetPlayerScores()
{
    std::vector<PlayerScore> ps;
    for(auto& [id, score] : playersScore)
        ps.push_back(score);

    return ps;
}

std::optional<PlayerScore> Scoreboard::GetPlayerScore(Entity::ID playerId)
{
    std::optional<PlayerScore> ps;
    if(Util::Map::Contains(playersScore, playerId))
        ps = playersScore[playerId];
    
    return ps;
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

std::vector<TeamScore> Scoreboard::GetTeamScores()
{
    std::vector<TeamScore> teamScores;
    for(auto [id, score] : this->teamsScore)
        teamScores.push_back(score);

    return teamScores;
}

std::optional<TeamScore> Scoreboard::GetWinner()
{
    std::optional<TeamScore> ret;

    auto scores = GetTeamScores();
    if(!scores.empty())
    {
        std::sort(scores.begin(), scores.end(), [](auto a, auto b){
            return a.score > b.score;
        });
        ret = scores[0];
    }

    return ret;
}

std::vector<PlayerScore> Scoreboard::GetTeamPlayers(Entity::ID teamId)
{
    std::vector<PlayerScore> tpScores;
    for(auto [id, pScore] : playersScore)
        if(pScore.teamId == teamId)
            tpScores.push_back(pScore);

    return tpScores;
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

Entity::ID GameMode::PlayerJoin(Entity::ID playerId, std::string name)
{
    auto teamId = OnPlayerJoin(playerId, name);
    scoreBoard.AddPlayer(playerId, teamId, name);

    return teamId;
}

void GameMode::PlayerLeave(Entity::ID playerId)
{
    scoreBoard.RemovePlayer(playerId);

    OnPlayerLeave(playerId);
}

void GameMode::PlayerDeath(Entity::ID killer, Entity::ID victim, Entity::ID killerTeamId)
{
    if(auto score = scoreBoard.GetPlayerScore(killer))
    {
        score->kills++;
        scoreBoard.SetPlayerScore(score.value());
    }

    if(auto score = scoreBoard.GetPlayerScore(victim))
    {
        score->deaths++;
        scoreBoard.SetPlayerScore(score.value());
    }

    OnPlayerDeath(killer, victim, killerTeamId);
}

bool GameMode::IsGameOver()
{
    auto teams = scoreBoard.GetTeamIDs();
    for(auto& tid : teams)
        if(scoreBoard.GetTeamScore(tid)->score >= maxScore)
            return true;

    return false;
}

std::vector<GameMode::Event> GameMode::PollEvents()
{
    auto events = std::move(this->events);
    return events;
}

void GameMode::AddEvent(Event event)
{
    events.push_back(event);
}

// GameMode - Non member functions

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
        //assertm(false, "This mode has not been implemented yet");
        gameMode = std::make_unique<TeamDeathMatch>();
        break;

    case GameMode::Type::DOMINATION:
        // assertm(false, "This mode has not been implemented yet");
        gameMode = std::make_unique<Domination>();
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

Entity::ID FreeForAll::OnPlayerJoin(Entity::ID id, std::string name)
{
    scoreBoard.AddPlayer(id, id, name);

    return id;
}

void FreeForAll::OnPlayerLeave(Entity::ID id)
{
    
}

void FreeForAll::OnPlayerDeath(Entity::ID killer, Entity::ID victim, Entity::ID killerTeamId)
{
    if(auto score = scoreBoard.GetPlayerScore(killer))
    {
        score->score++;
        scoreBoard.SetPlayerScore(score.value());

        auto teamScore = scoreBoard.GetTeamScore(killer);
        teamScore->score++;
        scoreBoard.SetTeamScore(teamScore.value());
    }
}

// GameMode - TeamGameMode

Entity::ID TeamGameMode::OnPlayerJoin(Entity::ID id, std::string name)
{
    auto teamId = BLUE_TEAM_ID;
    auto redTeamCount = scoreBoard.GetTeamPlayers(RED_TEAM_ID).size();
    auto blueTeamCount = scoreBoard.GetTeamPlayers(BLUE_TEAM_ID).size();

    if(redTeamCount < blueTeamCount)
        teamId = RED_TEAM_ID;
    else if(redTeamCount == blueTeamCount && redTeamCount > 0)
    {
        auto blueTeamScore = scoreBoard.GetTeamScore(BLUE_TEAM_ID);
        auto redTeamScore = scoreBoard.GetTeamScore(RED_TEAM_ID);

        if(redTeamScore->score < blueTeamScore->score)
            teamId = RED_TEAM_ID;
    }

    return teamId;
}

// GameMode - Team DeathMatch

void TeamDeathMatch::OnPlayerDeath(Entity::ID killer, Entity::ID victim, Entity::ID killerTeamId)
{
    if(auto score = scoreBoard.GetPlayerScore(killer))
    {
        score->score++;
        scoreBoard.SetPlayerScore(score.value());

        auto teamScore = scoreBoard.GetTeamScore(killer);
        teamScore->score++;
        scoreBoard.SetTeamScore(teamScore.value());
    }
}

// GameMode - Domination

const Util::Time::Seconds Domination::timeToCapture{10.0f};
const Util::Time::Seconds Domination::pointsTime{5.0f};

void Domination::Start(World world)
{
    auto map = world.map;

    auto domPoints = map->FindGameObjectByType(Entity::GameObject::DOMINATION_POINT);
    for(auto index : domPoints)
    {
        PointState pointState;
        pointState.index = index;
        pointState.capturedBy = TeamGameMode::NEUTRAL;
        pointState.timeLeft.SetDuration(timeToCapture);
        pointState.timeLeft.Start();

        pointsState[index] = pointState;
    }

    pointsTimer.SetDuration(pointsTime);
    pointsTimer.Start();
}

void Domination::Update(World world, Util::Time::Seconds deltaTime)
{
    auto map = world.map;

    // Check capture
    for(auto& [index, point] : pointsState)
    {
        for(auto& [pid, player] : world.players)
        {
            if(point.capturedBy == player->teamId)
                continue;

            if(IsPlayerInPointArea(world, index, player))
            {
                auto& timer = point.timeLeft;
                timer.Update(deltaTime);

                // Captured
                if(timer.IsDone())
                {
                    // Update owner
                    point.capturedBy = player->teamId;

                    // Add Event
                    Event event;
                    event.type = EventType::POINT_CAPTURED;
                    event.pointCaptured.capturedBy = point.capturedBy;
                    event.pointCaptured.pos = point.index;
                    AddEvent(event);

                    // Reset timers
                    point.timeLeft.Reset();
                    point.timeLeft.Start();
                }
            }
        }
    }

    // Give points
    pointsTimer.Update(deltaTime);
    if(pointsTimer.IsDone())
    {
        pointsTimer.Reset();
        pointsTimer.Start();
        for(auto& [index, point] : pointsState)
        {
            auto team = point.capturedBy;
            if(auto teamScore = scoreBoard.GetTeamScore(team))
            {
                teamScore->score++;
                scoreBoard.SetTeamScore(teamScore.value());
            }
        }
    }
}


bool Domination::IsPlayerInPointArea(World world, glm::ivec3 pos, Entity::Player* player)
{
    auto map = world.map;
    auto domPoint = map->GetGameObject(pos);
    auto scale = std::get<float>(domPoint->properties["Scale"].value);
    auto playerPos = player->GetTransform().position;
    auto realPos = Game::Map::ToRealPos(pos, map->GetBlockScale());
    auto distance = glm::length(realPos - playerPos);
    return distance <= scale;
}