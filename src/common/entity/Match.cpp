#include <Match.h>

#include <util/Container.h>

using namespace BlockBuster;

const std::string BlockBuster::GameMode::typeStrings[GameMode::Type::COUNT] = {"Free for All", "Team DeathMatch", "Domination", "Capture the Flag"};
const std::unordered_map<std::string, GameMode::Type> GameMode::stringTypes = {
    {"Free for All", GameMode::Type::FREE_FOR_ALL}, 
    {"Team DeathMatch", GameMode::Type::TEAM_DEATHMATCH}, 
    {"Domination", GameMode::Type::DOMINATION},
    {"Capture the Flag", GameMode::Type::CAPTURE_THE_FLAG}
};

// Gamemode

void GameMode::AddPlayer(Entity::ID id, std::string name)
{
    PlayerScore ps{id, name, 0, 0};
    scoreBoard[id] = ps;
}

std::optional<PlayerScore> GameMode::GetPlayerScore(Entity::ID id)
{
    std::optional<PlayerScore> ps;
    if(Util::Map::Contains(scoreBoard, id))
        ps = scoreBoard[id];
    
    return ps;
}

// Other

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

void FreeForAll::OnPlayerDeath(Entity::ID killer, Entity::ID victim)
{
    if(Util::Map::Contains(scoreBoard, killer))
        scoreBoard[killer].score++;
    if(Util::Map::Contains(scoreBoard, victim))
        scoreBoard[victim].deaths++;
}

bool FreeForAll::IsGameOver()
{
    for(auto& [id, score] : scoreBoard)
        if(score.score >= MAX_KILLS)
            return true;

    return false;
}

// Match

Match::Match(GameMode::Type type)
{
    switch (type)
    {
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
}

void Match::Start()
{
    waitTimer.Start();
}

void Match::Update(Log::Logger* logger, Util::Time::Seconds deltaTime)
{
    switch (state)
    {
    case WAITING_FOR_PLAYERS:
        waitTimer.Update(deltaTime);
        if(waitTimer.IsDone())
        {
            state = ON_GOING;
            logger->LogError("Match started");
            gameTimer.Start();
        }
        break;
    case ON_GOING:
        gameTimer.Update(deltaTime);
        if(gameMode->IsGameOver() || gameTimer.IsDone())
        {
            waitTimer.Start();
            state = ENDING;
        }
        break;
    case ENDING:
        waitTimer.Update(deltaTime);
        if(waitTimer.IsDone())
            state = ENDED;
        break;

    case ENDED:
        break;
    
    default:
        break;
    }
    gameMode->Update();
}