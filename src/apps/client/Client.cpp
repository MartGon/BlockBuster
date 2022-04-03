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

    menu = std::make_unique<MainMenu>(this);
    state = menu.get();
    state->Start();
    
    //LaunchGame("localhost", 8081, "Alpha2", "NULL PLAYER UUID", "Defu");
}

void Client::Shutdown()
{
    state->Shutdown();

    config.options["mapsFolder"] = mapMgr.GetMapsFolder().string();
}

void Client::Update()
{
    state->Update();
}

bool Client::Quit()
{
    return quit;
}

void Client::LaunchGame(std::string address, uint16_t port, std::string map, std::string playerUuid, std::string playerName)
{
    inGame = std::make_unique<InGame>(this, address, port, map, playerUuid, playerName);
    inGame->Start();
    state = inGame.get();
}

void Client::GoBackToMainMenu(bool onGoing)
{        
    inGame->Shutdown();
    inGame = nullptr;

    if(menu.get() == nullptr)
    {
        menu = std::make_unique<MainMenu>(this);
        menu->Start();
    }
    else if(onGoing)
        menu->LeaveGame();
    else if(menu->lobby)
        menu->UpdateGame(true);
    

    state = menu.get();
}

void Client::ApplyVideoOptions(App::Configuration::WindowConfig& winConfig)
{
    AppI::ApplyVideoOptions(winConfig);
    
    this->state->ApplyVideoOptions(winConfig);
}