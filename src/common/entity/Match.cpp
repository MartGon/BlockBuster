#include <Match.h>

using namespace BlockBuster;

// Match

void Match::Start(World world, GameMode::Type gameMode, uint8_t startingPlayers)
{
    this->gameMode = CreateGameMode(gameMode);
    this->gameMode->Start(world);
    this->startingPlayers = startingPlayers;
    timer.Start();
}

void Match::Update(World world, Util::Time::Seconds deltaTime)
{
    switch (state)
    {
    case WAITING_FOR_PLAYERS:
        {
            timer.Update(deltaTime);
            if(timer.IsDone() || world.players.size() == startingPlayers)
            {
                timer.SetDuration(startingTime);
                timer.Restart();
                
                EnterState(STARTING);
            }
        }
        break;
    case STARTING:
        timer.Update(deltaTime);
        if(timer.IsDone())
        {
            EnterState(ON_GOING);
            world.logger->LogError("Match started");

            timer.SetDuration(gameMode->GetDuration());
            timer.Start();
        }
        break;
    case ON_GOING:
        timer.Update(deltaTime);
        if(gameMode->IsGameOver() || timer.IsDone())
        {   
            timer.SetDuration(scoreboardTime);
            timer.Start();
            EnterState(ENDING);
        }
        else
            gameMode->Update(world, deltaTime);
        break;
    case ENDING:
        timer.Update(deltaTime);
        if(timer.IsDone())
            EnterState(ENDED);
        break;

    case ENDED:
        break;
    
    default:
        break;
    }
}

void Match::EnterState(StateType type)
{
    state = type;
    if(onEnterState)
        onEnterState(state);
}