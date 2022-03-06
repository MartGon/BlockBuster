#include <Client.h>

#include <util/Random.h>
#include <util/BBTime.h>

#include <math/Interpolation.h>

#include <debug/Debug.h>
#include <nlohmann/json.hpp>
#include <httplib/httplib.h>

#include <iostream>
#include <algorithm>

#include <GameState/InGame.h>
#include <GameState/MainMenu/MainMenu.h>

using namespace BlockBuster;

Client::Client(::App::Configuration config) : AppI{config}
{
}

void Client::Start()
{
    state = std::make_unique<MainMenu>(this);
    //nextState = std::make_unique<InGame>(this, "localhost", 8081);
    state->Start();
}

void Client::Shutdown()
{
    state->Shutdown();
}

void Client::Update()
{
    state->Update();

    if(nextState)
    {
        state->Shutdown();

        state = std::move(nextState);
        state->Start();
        nextState = nullptr;
    }
}

bool Client::Quit()
{
    return quit;
}

void Client::LaunchGame(std::string address, uint16_t port)
{
    nextState = std::make_unique<InGame>(this, address, port);
}