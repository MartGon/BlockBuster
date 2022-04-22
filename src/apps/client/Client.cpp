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
    // MapMgr
    auto mapsFolder = GetConfigOption("mapsFolder", "./maps");
    mapMgr.SetMapsFolder(mapsFolder);

    // Skybox
    try{
        skyboxShader = GL::Shader::FromFolder(config.openGL.shadersFolder, "skyboxVertex.glsl", "skyboxFrag.glsl");
    }
    catch(const std::runtime_error& e)
    {
        logger->LogCritical(e.what());
        quit = true;
        return;
    }

    resourcesDir = GetConfigOption("ResourcesFolder", RESOURCES_DIR);
    texturesDir = resourcesDir / "textures";
    GL::Cubemap::TextureMap map = {
        {GL::Cubemap::RIGHT, texturesDir / "right.jpg"},
        {GL::Cubemap::LEFT, texturesDir / "left.jpg"},
        {GL::Cubemap::TOP, texturesDir / "top.jpg"},
        {GL::Cubemap::BOTTOM, texturesDir / "bottom.jpg"},
        {GL::Cubemap::FRONT, texturesDir / "front.jpg"},
        {GL::Cubemap::BACK, texturesDir / "back.jpg"},
    };
    TRY_LOAD(skybox.Load(map, false));

    // MainMenu
    menu = std::make_unique<MainMenu>(this);
    state = menu.get();
    state->Start();
    
    LaunchGame("localhost", 8081, "Alpha3", "NULL PLAYER UUID", "Defu");
}

void Client::Shutdown()
{
    state->Shutdown();

    config.options["mapsFolder"] = mapMgr.GetMapsFolder().string();
    config.options["ResourcesFolder"] = resourcesDir.string();
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
    state = inGame.get();
    inGame->Start();
}

void Client::GoBackToMainMenu(bool onGoing)
{        
    inGame->Shutdown();
    inGame = nullptr;

    if(menu.get() == nullptr)
    {
        quit = true;
        return;
    }
    else if(onGoing)
        menu->LeaveGame();
    else if(menu->lobby)
        menu->UpdateGame(true);
    
    menu->ResetWindow();
    state = menu.get();
}

void Client::ApplyVideoOptions(App::Configuration::WindowConfig& winConfig)
{
    AppI::ApplyVideoOptions(winConfig);
    
    this->state->ApplyVideoOptions(winConfig);
}