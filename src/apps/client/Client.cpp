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

Client::Client(::App::Configuration config) : App{config}
{
}

void Client::Start()
{
    state = std::make_unique<MainMenu>(this);
    state->Start();
}

void Client::Shutdown()
{
    state->Shutdown();
}

void Client::Update()
{
    state->Update();
}

bool Client::Quit()
{
    return quit;
}