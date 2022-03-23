#include <Client.h>

#include <util/Random.h>
#include <util/BBTime.h>

#include <math/Interpolation.h>

#include <debug/Debug.h>
#include <nlohmann/json.hpp>
#include <httplib/httplib.h>

#include <iostream>
#include <algorithm>

#include <GameState/InGame/InGame.h>
#include <GameState/MainMenu/MainMenu.h>

using namespace BlockBuster;

Client::Client(::App::Configuration config) : AppI{config}
{
}

void Client::Start()
{
    auto mapsFolder = GetConfigOption("mapsFolder", "./maps");
    mapMgr.SetMapsFolder(mapsFolder);

    state = std::make_unique<MainMenu>(this);
    //state = std::make_unique<InGame>(this, "localhost", 8081, "Alpha2");
    state->Start();
    
    LaunchGame("localhost", 8081, "Alpha2");
}

void Client::Shutdown()
{
    state->Shutdown();

    config.options["mapsFolder"] = mapMgr.GetMapsFolder();
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

void Client::LaunchGame(std::string address, uint16_t port, std::string map)
{
    nextState = std::make_unique<InGame>(this, address, port, map);
}

void Client::ApplyVideoOptions(App::Configuration::WindowConfig& winConfig)
{
    AppI::ApplyVideoOptions(winConfig);
    
    this->state->ApplyVideoOptions(winConfig);
}