#include <GameMode.h>

#include <util/Container.h>

#include <collisions/Collisions.h>

using namespace BlockBuster;

const std::string BlockBuster::GameMode::typeStrings[GameMode::Type::COUNT] = {
    "", "Free for All", "Team DeathMatch", "Domination", "Capture the Flag"
    };
const std::unordered_map<std::string, GameMode::Type> GameMode::stringTypes = {
    {"Free for All", GameMode::Type::FREE_FOR_ALL}, 
    {"Team DeathMatch", GameMode::Type::TEAM_DEATHMATCH}, 
    {"Domination", GameMode::Type::DOMINATION},
    {"Capture the Flag", GameMode::Type::CAPTURE_THE_FLAG}
};

// Gamemode

Entity::ID GameMode::PlayerJoin(Entity::ID playerId, std::string name)
{
    // Note: We create it here, because OnPLayerJoin may add events
    targetedEvents[playerId] = std::vector<Event>();

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
        if(killer != victim)
            score->kills++;
        else
            score->kills--;
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

std::vector<Event> GameMode::PollEventMsgs(MsgType msgType, Entity::ID target)
{
    std::vector<Event> events;
    if(msgType == MsgType::BROADCAST)
        events = std::move(this->broadEvents);
    else if(msgType == MsgType::PLAYER_TARGET)
    {
        events = std::move(targetedEvents[target]);
    }

    return events;
}

void GameMode::WipeEvents()
{
    broadEvents.clear();
    targetedEvents.clear();
}

void GameMode::AddEvent(Event event)
{
    broadEvents.push_back(event);
}

void GameMode::AddEvent(Event event, Entity::ID playerTarget)
{
    targetedEvents[playerTarget].push_back(event);
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
        //assertm(false, "This mode has not been implemented yet");
        gameMode = std::make_unique<CaptureFlag>();
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
    gameModes[GameMode::Type::CAPTURE_THE_FLAG] = gameModes[GameMode::Type::FREE_FOR_ALL] && (!map.FindGameObjectByType(Entity::GameObject::FLAG_SPAWN_A).empty()
        || !map.FindGameObjectByType(Entity::GameObject::FLAG_SPAWN_B).empty() );

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
        pointState.capturedBy = TeamID::NEUTRAL;
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
            if(IsPlayerInPointArea(world, player, index) && !player->IsDead())
            {
                auto& timer = point.timeLeft;

                // Reset timer and leave
                if(point.capturedBy == player->teamId)
                {
                    timer.Restart();
                    continue;
                }

                timer.Update(deltaTime);

                // Captured
                if(timer.IsDone())
                {
                    // Update owner
                    point.capturedBy = (TeamID)player->teamId;

                    // Add Event
                    Event event;
                    event.type = EventType::POINT_CAPTURED;
                    event.pointCaptured.capturedBy = point.capturedBy;
                    event.pointCaptured.pos = point.index;
                    AddEvent(event);

                    // Reset timers
                    point.timeLeft.Restart();

                    // Increase player score
                    auto playerScore = scoreBoard.GetPlayerScore(player->id);
                    playerScore->score++;
                    scoreBoard.SetPlayerScore(playerScore.value());
                }
            }
        }
    }

    // Give points
    pointsTimer.Update(deltaTime);
    if(pointsTimer.IsDone())
    {
        pointsTimer.Restart();
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

bool Domination::IsPlayerInPointArea(World world, Entity::Player* player, glm::ivec3 pos)
{
    auto map = world.map;
    auto domPoint = map->GetGameObject(pos);
    auto scale = std::get<float>(domPoint->properties["Scale"].value);
    auto playerPos = player->GetTransform().position;
    auto realPos = Game::Map::ToRealPos(pos, map->GetBlockScale());

    auto aabb = Math::Transform{realPos, glm::vec3{0}, glm::vec3{scale * map->GetBlockScale()}};
    return Collisions::IsPointInAABB(playerPos, aabb);
}

// GameMode - Capture the flag

const Util::Time::Seconds CaptureFlag::timeToRecover{7.5f};
const Util::Time::Seconds CaptureFlag::timeToReset{45.0f};

void CaptureFlag::Start(World world)
{
    using namespace Entity;

    auto map = world.map;
    SpawnFlags(world, BLUE_TEAM_ID);
    SpawnFlags(world, RED_TEAM_ID);
    syncTimer.Start();
}

void CaptureFlag::Update(World world, Util::Time::Seconds deltaTime)
{
    auto& players = world.players;
    for(auto& [id, flag] : flags)
    {
        // Carried flags
        if(auto pid = flag.carriedBy)
        {
            auto playerId = pid.value();
            auto player = world.players[playerId];
            flag.pos = player->GetRenderTransform().position;
            auto capturePoints = GetCapturePoints(world, (TeamID)player->teamId);
            for(auto& captPoint : capturePoints)
            {
                auto realPos = Game::Map::ToRealPos(captPoint, world.map->GetBlockScale());
                if(IsPlayerInFlagArea(world, player, realPos, captureArea))
                    CaptureFlagBy(player, flag);
            }
        }
        // Dropped flags
        else
        {
            // Recover or Take
            for(auto& [pid, player] : players)
            {
                // Friendly player attempts to recover flag
                if(flag.teamId == player->teamId && !IsFlagInOrigin(flag) && IsPlayerInFlagArea(world, player, flag.pos, recoverArea))
                {
                    flag.recoverTimer.Update(deltaTime);
                    RecoverFlag(flag, player->id);
                }
                // Enemy player starts carrying
                else if(flag.teamId != player->teamId && !player->IsDead() && IsPlayerInFlagArea(world, player, flag.pos, captureArea))
                {
                    TakeFlag(flag, pid);
                    world.logger->LogDebug("[GameMode] Flag was taken");
                }
                    
            }

            // Reset flag
            if(!IsFlagInOrigin(flag))
            {
                flag.resetTimer.Update(deltaTime);
                if(flag.resetTimer.IsDone())
                    ResetFlag(flag);
            }
        }
    }

    // Sync flag state with clients
    syncTimer.Update(deltaTime);
    if(syncTimer.IsDone())
    {
        syncTimer.Restart();
        for(auto& [id, flag] : flags)
            AddEvent(GetFlagState(flag));
    }
}

bool CaptureFlag::IsPlayerInFlagArea(World world, Entity::Player* player, glm::vec3 flagPos, float area)
{
    auto map = world.map;
    auto playerPos = player->GetTransform().position;

    return Collisions::IsPointInSphere(playerPos, flagPos, area);
}

// Protected

Entity::ID CaptureFlag::OnPlayerJoin(Entity::ID playerId, std::string name)
{
    auto teamId = TeamGameMode::OnPlayerJoin(playerId, name);

    // Notify player just joined of taken and dropped flags
    for(auto& [flagId, flag] : flags)
        AddEvent(GetFlagState(flag), playerId);

    return teamId;
}

void CaptureFlag::OnPlayerDeath(Entity::ID killer, Entity::ID victim, Entity::ID killerTeamId)
{
    for(auto& [flagId, flag] : flags)
        if(flag.carriedBy.has_value() && flag.carriedBy == victim)
            DropFlag(flag);
}

void CaptureFlag::OnPlayerLeave(Entity::ID playerId)
{
    for(auto& [flagId, flag] : flags)
        if(flag.carriedBy.has_value() && flag.carriedBy == playerId)
            DropFlag(flag);
}

// Private

void CaptureFlag::SpawnFlags(World world, TeamID teamId)
{
    using namespace Entity;

    auto map = world.map;
    auto flagSpawns = GetCapturePoints(world, teamId);

    for(auto& pos : flagSpawns)
    {
        Flag flag;
        flag.flagId = flagId++;
        flag.pos = Game::Map::ToRealPos(pos, map->GetBlockScale());
        flag.teamId = teamId;
        flag.origin = flag.pos;
        flag.recoverTimer = Util::Timer{timeToRecover};
        flag.recoverTimer.Start();
        flag.resetTimer = Util::Timer{timeToReset};
        flag.resetTimer.Start();

        flags[flag.flagId] = flag;
    }
}

void CaptureFlag::CaptureFlagBy(Entity::Player* player, Flag& flag)
{
    // Increase team score
    auto teamScore = scoreBoard.GetTeamScore(player->teamId);
    teamScore->score++;
    scoreBoard.SetTeamScore(teamScore.value());

    // Increase player score
    auto playerScore = scoreBoard.GetPlayerScore(player->id);
    playerScore->score++;
    scoreBoard.SetPlayerScore(playerScore.value());

    ReturnFlag(flag);

    // Create event
    FlagEvent fe;
    fe.type = FlagEventType::FLAG_CAPTURED;
    fe.flagId = flag.flagId;
    fe.playerSubject = player->id;
    fe.pos = flag.pos;

    Event e;
    e.type = EventType::FLAG_EVENT;
    e.flagEvent = fe;

    AddEvent(e);
}

std::vector<glm::ivec3> CaptureFlag::GetCapturePoints(World world, TeamID teamId)
{
    using namespace Entity;
    auto goType = teamId == BLUE_TEAM_ID ? GameObject::Type::FLAG_SPAWN_A : GameObject::Type::FLAG_SPAWN_B;
    return world.map->FindGameObjectByType(goType);
}

bool CaptureFlag::IsFlagInOrigin(Flag& flag)
{
    glm::vec3 origin = flag.origin;
    float distance = glm::length(origin - flag.pos);
    return distance <= 0.25f;
}

Event CaptureFlag::GetFlagState(Flag& flag)
{
    // Flag State event
    FlagEvent fe;
    fe.type = FlagEventType::FLAG_STATE;
    fe.flagId = flag.flagId;
    if(flag.carriedBy)
        fe.playerSubject = flag.carriedBy.value();
    else
        fe.playerSubject = -1;
    fe.pos = flag.pos;
    fe.elapsed = flag.recoverTimer.GetElapsedTime();
    Event e;
    e.type = EventType::FLAG_EVENT;
    e.flagEvent = fe;

    return e;
}

void CaptureFlag::ResetFlag(Flag& flag)
{
    ReturnFlag(flag);

    // Flag Recovered event
    FlagEvent fe;
    fe.type = FlagEventType::FLAG_RESET;
    fe.flagId = flag.flagId;
    fe.pos = flag.pos;

    Event e;
    e.type = EventType::FLAG_EVENT;
    e.flagEvent = fe;

    AddEvent(e);
}

void CaptureFlag::RecoverFlag(Flag& flag, Entity::ID playerId)
{
    if(!flag.recoverTimer.IsDone())
        return;

    ReturnFlag(flag);

    // Flag Recovered event
    FlagEvent fe;
    fe.type = FlagEventType::FLAG_RECOVERED;
    fe.flagId = flag.flagId;
    fe.playerSubject = playerId;
    fe.pos = flag.pos;

    Event e;
    e.type = EventType::FLAG_EVENT;
    e.flagEvent = fe;

    AddEvent(e);
}

void CaptureFlag::TakeFlag(Flag& flag, Entity::ID playerId)
{
    flag.carriedBy = playerId;
    
    // Flag Taken event
    FlagEvent fe;
    fe.type = FlagEventType::FLAG_TAKEN;
    fe.flagId = flag.flagId;
    fe.playerSubject = playerId;
    fe.pos = flag.pos;
    Event e;
    e.type = EventType::FLAG_EVENT;
    e.flagEvent = fe;
    AddEvent(e);
}

void CaptureFlag::DropFlag(Flag& flag)
{
    // Flag Dropped event
    FlagEvent fe;
    fe.type = FlagEventType::FLAG_DROPPED;
    fe.flagId = flag.flagId;
    fe.playerSubject = flag.carriedBy.value();
    fe.pos = flag.pos;

    Event e;
    e.type = EventType::FLAG_EVENT;
    e.flagEvent = fe;

    AddEvent(e);

    // Remove from player
    flag.carriedBy.reset();

    // Restart timer
    flag.recoverTimer.Restart();
    flag.resetTimer.Restart();
}

void CaptureFlag::ReturnFlag(Flag& flag)
{
    flag.pos = flag.origin;
    flag.carriedBy.reset();
    flag.recoverTimer.Restart();
    flag.resetTimer.Restart();
}