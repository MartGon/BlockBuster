#include <Match.h>

using namespace BlockBuster;

// Match

void Match::Start(GameMode::Type gameMode)
{
    this->gameMode = CreateGameMode(gameMode);
    timer.Start();
}

void Match::Update(Log::Logger* logger, Util::Time::Seconds deltaTime)
{
    switch (state)
    {
    case WAITING_FOR_PLAYERS:
        timer.Update(deltaTime);
        if(timer.IsDone())
        {
            EnterState(ON_GOING);
            logger->LogError("Match started");

            timer.SetDuration(gameTime);
            timer.Start();
        }
        break;
    case ON_GOING:
        timer.Update(deltaTime);
        if(gameMode->IsGameOver() || timer.IsDone())
        {
            timer.SetDuration(waitTime);
            timer.Start();
            
            EnterState(ENDING);
        }
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

    if(gameMode.get())
        gameMode->Update();
}

void Match::EnterState(StateType type)
{
    state = type;
    if(onEnterState)
        onEnterState(state);
}