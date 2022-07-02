#include <GameState/InGame/InGame.h>

#include <Client.h>

#include <util/Random.h>
#include <util/BBTime.h>
#include <util/File.h>
#include <util/Container.h>

#include <math/Interpolation.h>

#include <debug/Debug.h>
#include <nlohmann/json.hpp>
#include <httplib/httplib.h>

#include <networking/Networking.h>
#include <game/Input.h>

#include <entity/Game.h>

#include <iostream>
#include <algorithm>

using namespace BlockBuster;
using namespace ::App::Client;

glm::vec4 InGame::teamColors[3] = {
    glm::vec4{0.065f, 0.072f, 0.8f, 1.0f}, // BLUE
    glm::vec4{0.8, 0.072f, 0.065f, 1.0f},   // RED
    Rendering::ColorU8ToFloat(192, 192, 192), // WHITE // GREY
};

glm::vec4 InGame::ffaColors[16] = {
    glm::vec4{0.065f, 0.072f, 0.8f, 1.0f}, // BLUE
    glm::vec4{0.8, 0.072f, 0.065f, 1.0f},  // RED
    Rendering::ColorU8ToFloat(0, 100, 0), // DARK GREEN
    Rendering::ColorU8ToFloat(255, 215, 0), // GOLD

    Rendering::ColorU8ToFloat(20, 20, 20), // BLACK
    Rendering::ColorU8ToFloat(192, 192, 192), // WHITE
    Rendering::ColorU8ToFloat(255, 165, 0), // ORANGE
    Rendering::ColorU8ToFloat(128, 0, 128), // PURPLE

    Rendering::ColorU8ToFloat(165, 42, 42), // BROWN
    Rendering::ColorU8ToFloat(0, 255, 0), // LIME
    Rendering::ColorU8ToFloat(0, 255, 255), // CYAN
    Rendering::ColorU8ToFloat(255, 20, 147), // PINK

    Rendering::ColorU8ToFloat(105, 105, 105), // GREY
    Rendering::ColorU8ToFloat(255, 0, 255), // MAGENTA
    Rendering::ColorU8ToFloat(188, 143, 143), // ROSY BROWN
    Rendering::ColorU8ToFloat(127, 255, 212), // AQUA MARINE
};

InGame::InGame(Client* client, std::string serverDomain, uint16_t serverPort, std::string map, std::string playerUuid, std::string playerName) : 
    GameState{client}, serverDomain{serverDomain}, serverPort{serverPort}, mapName{map}, playerUuid{playerUuid}, playerName{playerName},
    host{ENet::HostFactory::Get()->CreateHost(1, 2)}
{
}

// Public

void InGame::Start()
{
    // Window
    SDL_SetWindowResizable(this->client_->window_, SDL_TRUE);
    client_->ApplyVideoOptions(client_->config.window);

    // Load config
    LoadGameOptions();

    // GL features
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_STENCIL_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  

    // Shaders
    // Load shaders
    try{
        renderShader = GL::Shader::FromFolder(client_->config.openGL.shadersFolder, "renderVertex.glsl", "renderFrag.glsl");
        chunkShader = GL::Shader::FromFolder(client_->config.openGL.shadersFolder, "chunkVertex.glsl", "chunkFrag.glsl");
        billboardShader = GL::Shader::FromFolder(client_->config.openGL.shadersFolder, "billboardVertex.glsl", "billboardFrag.glsl");
        expShader = GL::Shader::FromFolder(client_->config.openGL.shadersFolder, "expVertex.glsl", "expFrag.glsl");
        textShader = GL::Shader::FromFolder(client_->config.openGL.shadersFolder, "textVertex.glsl", "textFrag.glsl");
        imgShader = GL::Shader::FromFolder(client_->config.openGL.shadersFolder, "imgVertex.glsl", "imgFrag.glsl");
    }
    catch(const std::runtime_error& e)
    {
        this->client_->logger->LogCritical(e.what());
        client_->quit = true;
        return;
    }

    // Textures
    renderMgr.Start();
    auto& texMgr = renderMgr.GetTextureMgr();
    texMgr.SetDefaultFolder(client_->texturesDir);

    // Audio
    InitAudio();

    // Map
    auto mapFolder = client_->mapMgr.GetMapFolder(mapName);
    auto mapFileName = mapName + ".bbm";
    LoadMap(mapFolder, mapFileName);

    // Models
    modelMgr.Start(renderMgr, renderShader, billboardShader);
    playerAvatar.SetMeshes(modelMgr.quad, modelMgr.cube, modelMgr.cylinder, modelMgr.slope);
    playerAvatar.Start(renderMgr, renderShader, billboardShader);
    fpsAvatar.SetMeshes(modelMgr.quad, modelMgr.cube, modelMgr.cylinder);
    fpsAvatar.Start(renderMgr, renderShader, renderShader);
    explosionMgr.Start(renderMgr, expShader);

    // UI
    inGameGui.Start();
    nameText = inGameGui.pixelFont->CreateText();

    // Camera
    auto winSize = client_->GetWindowSize();
    camera_.SetParam(Rendering::Camera::Param::ASPECT_RATIO, (float)winSize.x / (float)winSize.y);
    camera_.SetParam(Rendering::Camera::Param::FOV, client_->config.window.fov);
    camController_ = ::App::Client::CameraController{&camera_, {client_->window_, client_->io_}, ::App::Client::CameraMode::EDITOR};
    auto sensitivity = std::max(0.1f, std::stof(client_->GetConfigOption("Sensitivity", std::to_string(camController_.rotMod))));
    camController_.rotMod = sensitivity;
    #ifndef _DEBUG
        camController_.SetMode(::App::Client::CameraMode::FPS);
    #endif

    // Gameobjects
    InitGameObjects();

    // Match
    match.SetOnEnterState([this](Match::StateType type){
        this->OnEnterMatchState(type);
    });

    // Networking
    auto serverAddress = ENet::Address::CreateByDomain(serverDomain, serverPort).value();
    host.SetOnConnectCallback([this](auto id)
    {
        this->OnConnectToServer(id);
    });
    host.SetOnRecvCallback([this](auto id, auto channel, ENet::RecvPacket ePacket)
    {
        this->OnRecvPacket(id, channel, std::move(ePacket));
    });
    host.SetOnDisconnectCallback([this](auto id){
        if(this->match.GetState() != Match::ENDED && this->match.GetState() != Match::ENDING)
        {
            this->Exit();
            auto menu = this->client_->menu.get();
            menu->popUp.SetTitle("Connection error");
            menu->popUp.SetText("Disconnected from Server");
            menu->popUp.SetCloseable(true);
            menu->popUp.SetButtonVisible(true);
            menu->popUp.SetButtonCallback([menu](){
                menu->popUp.SetButtonVisible(false);
            });
        }
    });
    host.Connect(serverAddress);

    GetLogger()->LogInfo("Connecting to server at " + serverAddress.GetHostIP() + ":" + std::to_string(serverAddress.GetPort()));
    // Connect to server
    auto attempts = 0;
    while(!connected && attempts < 30)
    {
        Util::Time::Sleep(Util::Time::Millis{500});
        client_->logger->LogInfo("Connecting to server...");
        host.PollAllEvents();
        attempts++;
    }

    if(!connected)
    {
        GetLogger()->LogError("Could not connect to server: " + serverDomain + ":" + std::to_string(serverPort));
        GetLogger()->LogError("Quitting");
    }

    exit = !connected;
    client_->logger->Flush();

    // Apply options after initialization
    ApplyGameOptions(gameOptions);
}

void InGame::Update()
{
    simulationLag = serverTickRate;
    
    GetLogger()->LogInfo("Update rate(s) is: " + std::to_string(serverTickRate.count()));
    while(!exit)
    {
        preSimulationTime = Util::Time::GetTime();

        HandleSDLEvents();
        if(connected)
            RecvServerSnapshots();

        // Handle disconnection
        if(exit)
            break;

        while(simulationLag >= serverTickRate)
        {
            //HandleSDLEvents();
            DoUpdate(serverTickRate);

            if(connected)
                UpdateNetworking();

            simulationLag -= serverTickRate;
        }
        OnNewFrame(deltaTime);
        Render();
        UpdateAudio();

        deltaTime = (Util::Time::GetTime() - preSimulationTime);
        simulationLag += deltaTime;
        offsetTime += deltaTime;
        GetLogger()->LogDebug("Offset millis increased to " + std::to_string(offsetTime.count()));
        GetLogger()->LogDebug("Update: Delta time " + std::to_string(deltaTime.count()));
        GetLogger()->LogDebug("Update: Simulation lag " + std::to_string(simulationLag.count()));
    }

    ReturnToMainMenu();
}

void InGame::Shutdown()
{
    WriteGameOptions();
    audioMgr->Shutdown();
}

void InGame::ApplyVideoOptions(App::Configuration::WindowConfig& winConfig)
{
    auto winSize = client_->GetWindowSize();
    camera_.SetParam(camera_.ASPECT_RATIO, (float) winSize.x / (float) winSize.y);
    camera_.SetParam(Rendering::Camera::Param::FOV, winConfig.fov);
    camera_.SetParam(camera_.FAR_PLANE, (float)winConfig.renderDistance * camera_.FAR_PLANE_BASE_DISTANCE);
    GetLogger()->LogDebug("Applying vide options");
}

// Update

void InGame::DoUpdate(Util::Time::Seconds deltaTime)
{
    // Extra data: Respawns, etc
    respawnTimer.Update(deltaTime);
    match.Update(GetWorld(), deltaTime);
    if(match.GetState() == Match::ON_GOING)
        UpdateGameMode();

    // Check for gameObjects use range
    bool enabled = false;
    auto pPos = GetLocalPlayer().GetRenderTransform().position;
    for(auto [goPos, state] : gameObjectStates)
    {
        auto rPos = Game::Map::ToRealPos(goPos, map_.GetBlockScale());
        if(state.isActive && Collisions::IsPointInSphere(pPos, rPos, Entity::GameObject::ACTION_AREA))
        {
            auto go = map_.GetMap()->GetGameObject(goPos);
            inGameGui.EnableActionText(*go);
            enabled = true;
            break;
        }
    }
    if(!enabled)
        inGameGui.DisableActionText();
}

void InGame::OnEnterMatchState(Match::StateType type)
{
    auto mode = match.GetGameMode();
    switch (type)
    {
    case Match::StateType::WAITING_FOR_PLAYERS:
        {
            inGameGui.countdownText.SetText("Waiting for players");
            inGameGui.countdownText.SetIsVisible(true);
        }
        break;

    case Match::StateType::STARTING:
        {
            auto sound = Game::Sound::ANNOUNCER_SOUND_MODE_FFA;
            if(mode->GetType() == GameMode::Type::CAPTURE_THE_FLAG)
                sound = Game::Sound::ANNOUNCER_SOUND_MODE_CAPTURE_THE_FLAG;
            else if(mode->GetType() == GameMode::Type::DOMINATION)
                sound = Game::Sound::ANNOUNCER_SOUND_MODE_DOMINATION;
            else if(mode->GetType() == GameMode::Type::TEAM_DEATHMATCH)
                sound = Game::Sound::ANNOUNCER_SOUND_MODE_TEAM_DEATHMATCH;

            auto themeId = Util::Random::Uniform<int>(Game::Sound::MusicID::SPAWN_THEME_01_ID, Game::Sound::MusicID::SPAWN_THEME_04_ID);
            audioMgr->SetStreamSourceAudio(soundtrackSource, gallery.GetMusicId(static_cast<Game::Sound::MusicID>(themeId)));
            audioMgr->PlayStreamSource(soundtrackSource);
            PlayAnnouncerAudio(sound);

            // GUI
            inGameGui.countdownText.SetIsVisible(true);
        }
        break;
    case Match::StateType::ON_GOING:
        {
            inGameGui.countdownText.SetIsVisible(false);
            inGameGui.gameTimeText.SetIsVisible(true);
            inGameGui.EnableScore();
        }
        break;
    
    case Match::StateType::ENDING:
        {
            inGameGui.EnableWinnerText(true);

            auto scoreBoard = mode->GetScoreboard();
            auto winner = scoreBoard.GetWinner().value();
            if(mode->GetType() != GameMode::Type::FREE_FOR_ALL)
            {
                auto sound = winner.teamId == BLUE_TEAM_ID ? Game::Sound::ANNOUNCER_SOUND_BLUE_TEAM_WINS : Game::Sound::ANNOUNCER_SOUND_RED_TEAM_WINS;
                PlayAnnouncerAudio(sound);
            }

            bool isWinner = winner.teamId == GetLocalPlayer().teamId;
            if(isWinner)
            {
                auto themeId = Util::Random::Uniform<int>(Game::Sound::MusicID::VICTORY_THEME_01_ID, Game::Sound::MusicID::VICTORY_THEME_02_ID);
                audioMgr->SetStreamSourceAudio(soundtrackSource, gallery.GetMusicId(static_cast<Game::Sound::MusicID>(themeId)));
                audioMgr->PlayStreamSource(soundtrackSource);
            }

            predictionHistory_.Clear();
        }
        break;

    case Match::StateType::ENDED:
        inGameGui.EnableWinnerText(false);
        client_->SetMouseGrab(false);
        break;

    default:
        break;
    }
}

void InGame::UpdateGameMode()
{
    auto mode = match.GetGameMode();
    mode->WipeEvents();
    auto map = map_.GetMap();

    switch (mode->GetType())
    {
    case GameMode::Type::DOMINATION:
        {
            auto domination = static_cast<Domination*>(mode);
            auto& player = GetLocalPlayer();
            auto world = GetWorld();
            
            bool wasInArea = false;
            for(auto& [pos, point] : domination->pointsState)
            {
                if(domination->IsPlayerInPointArea(world, &player, pos))
                {
                    inGameGui.EnableCapturingText(true);
                    auto percent = point.GetCapturePercent();

                    if(point.capturedBy == player.teamId)
                    {
                        percent = 100.0f;
                        inGameGui.capturingText.SetText("CAPTURED");
                    }
                    else
                        inGameGui.capturingText.SetText("CAPTURING");

                    inGameGui.SetCapturePercent(percent);

                    wasInArea = true;
                }
            }

            if(!wasInArea)
                inGameGui.EnableCapturingText(false);
        }
        break;

     case GameMode::Type::CAPTURE_THE_FLAG:
        {
            auto captureFlag = static_cast<CaptureFlag*>(mode);
            auto& player = GetLocalPlayer();
            auto world = GetWorld();
            
            bool wasInArea = false;
            bool isCarrying = false;
            for(auto& [flagId, flag] : captureFlag->flags)
            {
                bool friendlyFlag = flag.teamId == player.teamId;
                bool inArea = captureFlag->IsPlayerInFlagArea(world, &player, flag.pos, captureFlag->recoverArea);
                bool dropped = !captureFlag->IsFlagInOrigin(flag) && !flag.carriedBy.has_value();
                if(friendlyFlag && dropped && inArea)
                {
                    inGameGui.EnableCapturingText(true);
                    auto percent = flag.GetRecoverPercent();
                    inGameGui.capturingText.SetText("RECOVERING");
                    inGameGui.SetCapturePercent(percent);

                    wasInArea = true;
                }

                if(flag.carriedBy.has_value() && flag.carriedBy.value() == player.teamId)
                    isCarrying = true;
            }

            if(!wasInArea)
                inGameGui.EnableCapturingText(false);

            inGameGui.flagIconImg.SetIsVisible(isCarrying);
        }
        break;
    
    default:
        break;
    }
}

void InGame::OnNewFrame(Util::Time::Seconds deltaTime)
{
    EntityInterpolation();
    SmoothPlayerMovement();

    auto& player = GetLocalPlayer();

    // Update shield
    if(!player.IsDead())
        player.health = pController.UpdateShield(player.health, player.dmgTimer, deltaTime);

    // Set camera in player
    auto camMode = camController_.GetMode();
    if(camMode == CameraMode::FPS)
    {
        auto camPos = player.GetFPSCamPos();
        camera_.SetPos(camPos);
    }
    else
        camController_.Update();

    // Update animations
    fpsAvatar.Update(deltaTime);
    for(auto& [playerId, playerState] : playerModelStateTable)
    {
        playerState.deathPlayer.Update(deltaTime);
        playerState.shootPlayer.Update(deltaTime);
    }
    explosionMgr.Update(deltaTime);

    // Update camera
    if(auto lastPred = predictionHistory_.Back())
        UpdateCamera(lastPred->inputReq.playerInput);

    // Rellocate camera
    if(player.IsDead() && Util::Map::Contains(playerTable, killerId) && killerId != playerId)
    {
        // Look to killer during death
        auto& killer = playerTable[killerId];
        auto pos = killer.GetRenderTransform().position;
        camera_.SetTarget(pos);
    }

    // AUDIO

    // Update announcer pos
    auto pPos = player.GetRenderTransform().position;
    audioMgr->SetSourceTransform(announcerSource, pPos);

    // Update player source pos
    audioMgr->SetSourceTransform(playerSource, pPos);
    audioMgr->SetSourceTransform(playerReloadSource, pPos);
    audioMgr->SetSourceTransform(playerDmgSource, pPos);
    
    // Update players source Pos
    for(auto& [id, ed] : playersExtraData)
    {
        auto pPos = playerTable[id].GetRenderTransform().position;
        audioMgr->SetSourceTransform(ed.dmgSourceId, pPos);
        audioMgr->SetSourceTransform(ed.shootSourceId, pPos);
    }
}

// Exit

void InGame::ReturnToMainMenu()
{
    auto onGoing = match.GetState() != Match::StateType::ENDED;
    client_->GoBackToMainMenu(onGoing);
}

void InGame::Exit()
{
    exit = true;
    client_->SetMouseGrab(false);
}

// Input

void InGame::HandleSDLEvents()
{
    SDL_Event e;
    
    while(SDL_PollEvent(&e) != 0)
    {
        ImGui_ImplSDL2_ProcessEvent(&e);

        switch(e.type)
        {
        case SDL_QUIT:
            Exit();
            client_->quit = true;
            break;
        case SDL_KEYDOWN:
            if(e.key.keysym.sym == SDLK_p)
            {
                using namespace ::App::Client;
                auto mode = this->camController_.GetMode();
                auto nextMode = mode == CameraMode::EDITOR ? CameraMode::FPS : CameraMode::EDITOR;
                #ifdef _DEBUG
                    this->camController_.SetMode(nextMode);
                #endif
            }
            if(e.key.keysym.sym == SDLK_ESCAPE)
            {
                if(!inGameGui.IsMenuOpen())
                    inGameGui.OpenMenu();
                else 
                    inGameGui.CloseMenu();
            }
            if(e.key.keysym.sym == SDLK_TAB)
            {
                inGameGui.showScoreboard = true;
            }
            break;
        case SDL_KEYUP:
            if(e.key.keysym.sym == SDLK_TAB)
            {
                inGameGui.showScoreboard = false;
            }
        case SDL_MOUSEBUTTONDOWN:
            {
            }
            break;
        }
        
        bool hasNotEnded = match.GetState() != Match::StateType::ENDED && match.GetState() != Match::StateType::ENDING;
        if(!inGameGui.IsMenuOpen() && hasNotEnded)
            camController_.HandleSDLEvent(e);
    }
}

void InGame::UpdateCamera(Entity::PlayerInput input)
{
    // Zoom
    auto& player = GetLocalPlayer();
    auto weapon = player.GetCurrentWeapon();
    auto canZoom = weapon.state == Entity::Weapon::State::IDLE || weapon.state == Entity::Weapon::State::SHOOTING;

    auto offset = (float)deltaTime.count() * zoomSpeed;
    if(input[Entity::Inputs::ALT_SHOOT] && canZoom)
        zoomMod = std::min(zoomMod + offset, 1.0f);
    else
        zoomMod = std::max(zoomMod - offset, 0.0f);

    auto wepType = Entity::WeaponMgr::weaponTypes.at(weapon.weaponTypeId);
    auto zoom = 1.0f + (wepType.zoomLevel - 1.0f) * zoomMod;
    camera_.SetZoom(zoom);

    if(gameOptions.dynamicSensitivity)
        camController_.rotMod = gameOptions.sensitivity * (1 / std::max(0.1f, zoom));
}

void InGame::WeaponRecoil()
{
    auto camRot = camera_.GetRotationDeg();
    auto sideRecoil = Util::Random::Normal(0.0f, 1.0f);
    auto strength = Entity::WeaponMgr::weaponTypes.at(GetLocalPlayer().GetCurrentWeapon().weaponTypeId).recoil;
    auto recoil = glm::vec2{-1.0f, sideRecoil} * strength;
    auto newRot = camRot + recoil;

    camera_.SetRotationDeg(newRot.x, newRot.y);
}

// World

void InGame::InitGameObjects()
{
    auto map = map_.GetMap();
    auto criteria = [this](glm::ivec3 pos, Entity::GameObject& go)
    {
        return go.IsInteractable();
    };
    auto goIndices = map->FindGameObjectByCriteria(criteria);
    for(auto goIndex : goIndices)
    {
        gameObjectStates[goIndex] = Entity::GameObject::State{true};
    }
}

Entity::Player& InGame::GetLocalPlayer()
{
    return playerTable[playerId];
}

Entity::ID InGame::GetPlayerTeam(Entity::ID playerId)
{
    Entity::ID teamId = 0;
    auto scoreBoard = match.GetGameMode()->GetScoreboard();
    if(auto score = scoreBoard.GetPlayerScore(playerId))
        teamId = score->teamId;

    return teamId;
}

void InGame::OnGrenadeExplode(Entity::Projectile& grenade)
{
    auto pos = grenade.GetPos();
    explosionMgr.CreateExplosion(pos);

    // Play sound effect
    auto source = grenadeSources.Get();
    audioMgr->SetSourceTransform(source, pos);
    audioMgr->PlaySource(source);
}

void InGame::OnLocalPlayerShot()
{
    auto& player = GetLocalPlayer();
    auto& weapon = player.GetCurrentWeapon();
    
    auto audioId = gallery.GetWepSoundID(weapon.weaponTypeId);
    audioMgr->SetSourceAudio(playerSource, audioId);
    audioMgr->SetSourceTransform(playerSource, player.GetRenderTransform().position);
    audioMgr->PlaySource(playerSource);

    auto projViewMat = camera_.GetProjViewMat();
    Collisions::Ray ray = Collisions::ScreenToWorldRay(projViewMat, glm::vec2{0.5f, 0.5f}, glm::vec2{1.0f});
    auto collision = Game::CastRayFirst(map_.GetMap(), ray, map_.GetBlockScale());
    if(collision.intersection.intersects && collision.block->type != Game::SLOPE)
    {
        auto colPoint = collision.intersection.colPoint;
        auto normal = collision.intersection.normal;

        auto up = Rendering::Camera::UP;        
        if(abs(normal.y) == 1.0f)
            up = glm::vec3{0.0f, 0.0f, 1.0f};

        auto orientation = glm::lookAt(glm::vec3{0.0f}, normal, up);
        auto translate = glm::translate(glm::mat4{1.0f}, colPoint + (normal * 0.05f));
        auto mat = translate * orientation;
        decalTransforms.PushBack(mat);
    }

    fpsAvatar.PlayShootAnimation(); 
    WeaponRecoil();
}

void InGame::OnLocalPlayerReload()
{
    auto& player = GetLocalPlayer();
    auto weapon = player.GetCurrentWeapon();
    auto audioId = gallery.GetWepReloadSoundId(weapon.weaponTypeId);
    audioMgr->SetSourceAudio(playerReloadSource, audioId);
    audioMgr->PlaySource(playerReloadSource);
}

void InGame::OnLocalPlayerTakeDmg()
{
    auto soundId = Util::Random::Uniform(0u, 1u) + Game::Sound::BULLET_HIT_METAL_1;
    auto audioId = gallery.GetSoundId((Game::Sound::SoundID)soundId);
    audioMgr->SetSourceAudio(playerDmgSource, audioId);
    audioMgr->PlaySource(playerDmgSource);
}

World InGame::GetWorld()
{
    World world;

    world.map = map_.GetMap();
    for(auto& [pid, player] : playerTable)
        world.players[pid] = &player;

    world.logger = GetLogger();

    return world;
}

// Rendering

void InGame::Render()
{
    // Clear Buffer
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    DrawScene();
    inGameGui.DrawGUI(textShader);

    // Swap buffers
    SDL_GL_SwapWindow(client_->window_);

    // Max FPS Correction
    auto renderTime = Util::Time::GetTime() - preSimulationTime;
    if(renderTime < minFrameInterval)
    {
        Util::Time::Seconds diff = minFrameInterval - renderTime;
        //Util::Time::Sleep(diff);
    }
}

void InGame::DrawScene()
{
    // GameMode
    bool isFFA = match.GetGameMode()->GetType() == GameMode::FREE_FOR_ALL;

    // Bind textures
    map_.tPalette.GetTextureArray()->Bind(GL_TEXTURE0);
    chunkShader.SetUniformInt("textureArray", 0);
    map_.cPalette.GetTextureArray()->Bind(GL_TEXTURE1);
    chunkShader.SetUniformInt("colorArray", 1);

    // Draw map
    auto view = camera_.GetProjViewMat();
    map_.Draw(chunkShader, view);

    // Draw players
    for(auto [playerId, player] : playerTable)
    {
        if(this->playerId == playerId && camController_.GetMode() == CameraMode::FPS)
            continue;

        // Apply changes to model
        auto playerState = playerModelStateTable[playerId];
        auto playerT = player.GetTransform();
        playerAvatar.SetArmsPivot(playerState.armsPivot);
        playerAvatar.SetFlashesActive(playerState.leftFlashActive);
        auto cPitch = std::min(std::max(playerT.rotation.x, 60.0f), 120.0f);
        auto pitch = -(cPitch- 90.0f);
        playerAvatar.RotateArms(pitch);
        auto lmd = GetLastMoveDir(playerId);
        playerAvatar.SteerWheels(lmd, playerT.rotation.y);
        auto flagColor = inGameGui.GetOppositeColor(player.teamId);
        playerAvatar.SetFlagActive(playerState.flagCarrying, flagColor);

        // Draw model
        playerT = player.GetRenderTransform();
        playerT.scale *= playerState.gScale;
        auto t = playerT.GetTransformMat();
        auto transform = view * t;
        auto color = isFFA ? ffaColors[player.teamId] : teamColors[player.teamId];
        playerAvatar.SetColor(color);
        playerAvatar.Draw(transform);
        
        // Draw playerName
        DrawPlayerName(playerId);

    #ifdef _DEBUG
        // Draw player move collision box
        Math::Transform boxTf = pController.GetECB();
        //DrawCollisionBox(view, boxTf);

        // Draw player hitbox
        auto playerHitbox = Entity::Player::GetHitBox();
        using HitBoxType = Entity::Player::HitBoxType;
        for(uint8_t i = HitBoxType::HEAD; i < HitBoxType::WHEELS; i++)
        {
            auto t = playerHitbox[i];
            t.position += playerT.position;
            t.rotation.y = playerT.rotation.y;

            DrawCollisionBox(view, t);
        }
        auto wt = playerHitbox[HitBoxType::WHEELS];
        wt.position += playerT.position;
        wt.rotation.y = Entity::Player::GetWheelsRotation(lmd, playerT.rotation.y + 90.0f);

        DrawCollisionBox(view, wt);
    #endif
    }
    prevPlayerTable = playerTable;

    // Draw skybox
    client_->skybox.Draw(client_->skyboxShader, camera_.GetViewMat(), camera_.GetProjMat());

    // Draw models
    DrawGameObjects();
    DrawModeObjects();
    DrawProjectiles();
    DrawDecals();
    explosionMgr.DrawExplosions();
    renderMgr.Render(camera_);

    // Draw fpsModel, always rendered last
    auto proj = camera_.GetProjMat();
    auto& player = playerTable[playerId];
    auto color = isFFA ? ffaColors[player.teamId] : teamColors[player.teamId];
    fpsAvatar.Draw(proj, color);
    renderMgr.Render(camera_);
}

void InGame::DrawGameObjects()
{
    auto view = camera_.GetProjViewMat();
    auto map = map_.GetMap();
    auto blockScale = map->GetBlockScale();

    for(auto& [pos, state] : gameObjectStates)
    {
        if(!state.isActive)
            continue;

        // Calc. Pos and flags
        auto go = map->GetGameObject(pos);
        auto rPos = Game::Map::ToRealPos(pos, blockScale);
        rPos.y -= (blockScale / 2.0f);
        
        // Billboard icon
        auto iconPos = rPos + glm::vec3{0.0f, 1.5f, 0.0f};
        auto renderflags = Rendering::RenderMgr::RenderFlags::NO_FACE_CULLING;
        glm::vec2 scale{1.25f};
        if(go->type == Entity::GameObject::Type::WEAPON_CRATE)
        {
            auto wepId = static_cast<Entity::WeaponTypeID>(std::get<int>(go->properties["Weapon ID"].value));
            scale = glm::vec2{3.14f, 1.0f};
            modelMgr.DrawWepBillboard(wepId, iconPos, 0.0f, scale, glm::vec4{1.0f}, renderflags);
        }
        else if(go->type == Entity::GameObject::Type::HEALTHPACK)
            modelMgr.DrawBillboard(Game::Models::RED_CROSS_ICON_ID, iconPos, 0.0f, scale, glm::vec4{1.0f}, renderflags);

        // Draw gameObject
        Math::Transform t{rPos, glm::vec3{0.0f}, glm::vec3{1.0f}};
        auto tMat = view * t.GetTransformMat();
        modelMgr.DrawGo(go->type, tMat);
    }

    auto teleports = map->FindGameObjectByCriteria([](auto pos, Entity::GameObject go){
        return go.type == Entity::GameObject::TELEPORT_DEST || go.type == Entity::GameObject::TELEPORT_ORIGIN;
    });

    for(auto pos : teleports)
    {
        auto go = map->GetGameObject(pos);

        auto rPos = Game::Map::ToRealPos(pos, blockScale);
        rPos.y -= (blockScale / 2.0f);
        Math::Transform t{rPos, glm::vec3{0.0f}, glm::vec3{1.0f}};
        auto tMat = view * t.GetTransformMat();
        modelMgr.DrawGo(go->type, tMat);
    }
}

void InGame::DrawModeObjects()
{
    auto view = camera_.GetProjViewMat();
    auto mode = match.GetGameMode();
    auto map = map_.GetMap();

    std::vector<glm::ivec3> goIndices;
    auto blockScale = map_.GetBlockScale();
    switch (mode->GetType())
    {
    case GameMode::Type::DOMINATION:
        {
            goIndices = map->FindGameObjectByType(Entity::GameObject::DOMINATION_POINT);
            auto domination = static_cast<Domination*>(match.GetGameMode());
            
            for(auto& [pos, point] : domination->pointsState)
            {
                auto color = teamColors[point.capturedBy];
                auto iconPos = Game::Map::ToRealPos(pos, map->GetBlockScale()) + glm::vec3{0.0f, 3.0f, 0.0f};
                if(!Collisions::IsPointInSphere(camera_.GetPos(), iconPos, 5.0f))
                {
                    auto renderflags = Rendering::RenderMgr::RenderFlags::NO_FACE_CULLING | Rendering::RenderMgr::RenderFlags::IGNORE_DEPTH;
                    modelMgr.DrawBillboard(Game::Models::FLAG_ICON_ID, iconPos, 0.0f, glm::vec2{2.f}, color, renderflags);
                }
            }
        }
        break;

    case GameMode::Type::CAPTURE_THE_FLAG:
        {
            std::vector<glm::ivec3> flagSpawns[2];
            flagSpawns[TeamID::BLUE_TEAM_ID] = map->FindGameObjectByType(Entity::GameObject::FLAG_SPAWN_A);
            flagSpawns[TeamID::RED_TEAM_ID] = map->FindGameObjectByType(Entity::GameObject::FLAG_SPAWN_B);

            for(auto teamId = 0; teamId < TeamID::NEUTRAL; teamId++)
            {
                auto& fs = flagSpawns[teamId];
                goIndices.insert(goIndices.begin(), fs.begin(), fs.end());

                for(auto pos : fs)
                {
                    auto color = teamColors[teamId];
                    auto iconPos = Game::Map::ToRealPos(pos, map->GetBlockScale()) + glm::vec3{0.f, 5.0f, 0.0f};
                    if(!Collisions::IsPointInSphere(camera_.GetPos(), iconPos, 5.0f))
                    {
                        auto renderflags = Rendering::RenderMgr::RenderFlags::NO_FACE_CULLING | Rendering::RenderMgr::RenderFlags::IGNORE_DEPTH;
                        modelMgr.DrawBillboard(Game::Models::FLAG_ICON_ID, iconPos, 0.0f, glm::vec2{2.f}, color, renderflags);
                    }
                }
            }

            auto captureFlag = static_cast<CaptureFlag*>(match.GetGameMode());
            for(auto& [id, flag] : captureFlag->flags)
            {
                if(flag.carriedBy.has_value())
                    continue;
                
                Math::Transform t{flag.pos, glm::vec3{0.0f}, glm::vec3{2.0f}};
                auto tMat = view * t.GetTransformMat();
                auto model = static_cast<Rendering::Model*>(modelMgr.GetModel(Game::Models::FLAG_MODEL_ID));

                const auto CLOTH_SM_ID = 1;
                model->GetSubModel(CLOTH_SM_ID)->painting.color = teamColors[flag.teamId];
                modelMgr.Draw(Game::Models::FLAG_MODEL_ID, tMat);
            }
        }
        break;
    
    default:
        break;
    }

    for(auto goPos : goIndices)
    {
        auto go = map->GetGameObject(goPos);
        auto rPos = Game::Map::ToRealPos(goPos, blockScale);
        rPos.y -= (blockScale / 2.0f);

        Math::Transform t{rPos, glm::vec3{0.0f}, glm::vec3{1.0f}};
        auto tMat = view * t.GetTransformMat();
        modelMgr.DrawGo(go->type, tMat);
    }
}

void InGame::DrawProjectiles()
{
    auto view = camera_.GetProjViewMat();

    for(auto& [id, projectile] : projectiles)
    {
        Math::Transform t{projectile->GetPos(), projectile->GetRotation(), glm::vec3{1.0f}};
        
        auto type = projectile->GetType();
        if(type == Entity::Projectile::GRENADE)
        {
            auto tMat = view * t.GetTransformMat();
            modelMgr.Draw(Game::Models::GRENADE_MODEL_ID, tMat);
        }
        else if(type == Entity::Projectile::ROCKET)
        {
            auto translation = glm::translate(glm::mat4{1.0f}, projectile->GetPos());
            auto dir = glm::normalize(projectile->GetVelocity());
            auto lookPoint = t.position - dir;
            auto rotation = glm::inverse(glm::lookAt(t.position, lookPoint, glm::vec3{0.0f, 1.0f, 0.0f})) 
                * glm::rotate(glm::mat4{1.0f}, glm::radians(90.0f), glm::vec3{1.0f, 0.0f, 0.0f});
            rotation = glm::mat4{glm::mat3{rotation}};
            auto tMat = translation * rotation;
            tMat = view * tMat;
            modelMgr.Draw(Game::Models::ROCKET_MODEL_ID, tMat);
        }

        #ifdef _DEBUG
            t.scale = projectile->GetScale();
            DrawCollisionBox(view, t);
        #endif
    }
}

void InGame::DrawDecals()
{
    auto view = camera_.GetProjViewMat();

    auto size = decalTransforms.GetSize();
    for(auto i = 0; i < size; i++)
    {
        auto decalMat = decalTransforms.At(i).value();
        auto mat = view * decalMat;
        modelMgr.Draw(Game::Models::DECAL_MODEL_ID, mat);
    }
}

void InGame::DrawPlayerName(Entity::ID playerId)
{
    auto& player = playerTable[playerId];
    if(player.IsDead())
        return;

    if(auto mode = match.GetGameMode())
    {
        auto& scoreBoard = mode->GetScoreboard();
        if(auto ps = scoreBoard.GetPlayerScore(playerId))
        {
            nameText.SetText(ps->name);

            // Render to texture
            auto& ed = playersExtraData[playerId];
            
            ed.frameBuffer.Bind();
                auto texSize = ed.frameBuffer.GetTextureSize();
                glViewport(0, 0, texSize.x, texSize.y);
                glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                nameText.SetAnchorPoint(GUI::AnchorPoint::DOWN_LEFT_CORNER);
                nameText.SetColor(glm::vec4{1.0f});
                nameText.SetScale(1.25f);
                auto size = nameText.GetSize();
                nameText.SetOffset(glm::ivec2{texSize.x / 2 - size.x / 2, 0});
                nameText.Draw(textShader, texSize);

                auto enemyWepId = player.GetCurrentWeapon().weaponTypeId;
                auto tex = modelMgr.GetWepIconTex(enemyWepId);
                wepImage.SetAnchorPoint(GUI::AnchorPoint::CENTER_UP);
                wepImage.SetTexture(tex);
                wepImage.SetParent(&nameText);
                auto wepSize = wepImage.GetSize();
                wepImage.SetOffset(glm::ivec2{-wepSize.x / 2, 5});
                wepImage.Draw(imgShader, texSize);
            ed.frameBuffer.Unbind();

            auto winSize = client_->GetWindowSize();
            glViewport(0, 0, winSize.x, winSize.y);

            // Draw name billboard
            auto pPos = player.GetRenderTransform().position;
            auto bPos = pPos + glm::vec3{0.0f, 2.75f, 0.0f};
            if(ed.nameBillboard)
                ed.nameBillboard->Draw(bPos, 0.0f, glm::vec2{1.6f, 0.9f} * 2.0f, glm::vec4{1.0f});
        }
    }
}

void InGame::DrawCollisionBox(const glm::mat4& viewProjMat, Math::Transform box)
{
    auto mat = viewProjMat * box.GetTransformMat();
    renderShader.SetUniformMat4("transform", mat);
    modelMgr.cube.Draw(renderShader, glm::vec4{1.0f}, GL_LINE);
}

// Audio

void InGame::InitAudio()
{
    using namespace Game::Sound;

    // Set up gallery
    audioMgr = audioMgr->Get();
    audioMgr->Init();
    std::filesystem::path audioFolder = client_->resourcesDir / "audio";
    gallery.SetDefaultFolder(audioFolder);
    gallery.Start();

    // Play Spawn Theme
    soundtrackSource = audioMgr->CreateStreamSource();
    Audio::AudioSource::Params audioParams;
    audioParams.gain = 0.5f;
    audioMgr->SetStreamSourceParams(soundtrackSource, audioParams);
    audioMgr->SetStreamSourceAudio(soundtrackSource, gallery.GetMusicId(MusicID::SPAWN_THEME_01_ID));

    // Player source
    playerSource = audioMgr->CreateSource();
    playerReloadSource = audioMgr->CreateSource();
    playerDmgSource = audioMgr->CreateSource();

    // Set grenade sources
    for(auto i = 0; i < grenadeSources.GetSize(); i++)
    {
        auto sourceId = audioMgr->CreateSource();
        grenadeSources.Get() = sourceId;

        audioMgr->SetSourceAudio(sourceId, gallery.GetSoundId(SoundID::GRENADE_SOUND_ID));
        audioMgr->SetSourceParams(sourceId, glm::vec3{0.0f}, 0.0f, false, 8.0f, 1.5f);
    }

    // Announcer source
    announcerSource = audioMgr->CreateSource();
}

void InGame::UpdateAudio()
{
    audioMgr->SetListenerTransform(camera_.GetPos(), camera_.GetFront());
    audioMgr->Update();
}

void InGame::PlayAnnouncerAudio(Game::Sound::AnnouncerSoundID asid)
{
    auto audioId = gallery.GetAnnouncerSoundId(asid);
    audioMgr->SetSourceAudio(announcerSource, audioId);
    audioMgr->PlaySource(announcerSource);
}

// Map
// TODO: Redundancy with Project::Load
void InGame::LoadMap(std::filesystem::path mapFolder, std::string fileName)
{
    using namespace Util::File;
    auto filepath = mapFolder / fileName;
    std::fstream file{filepath, file.binary | file.in};
    if(!file.is_open())
    {
        GetLogger()->LogError("Could not open file " + filepath.string());
        std::exit(-1);
        return;
    }

    auto magic = ReadFromFile<int>(file);
    if(magic != Game::Map::Map::magicNumber)
    {
        GetLogger()->LogError("Wrong format for file " + filepath.string());
        return;
    }

    // Load map
    auto bufferSize = ReadFromFile<uint32_t>(file);
    Util::Buffer buffer = ReadFromFile(file, bufferSize);
    map_ = App::Client::Map::FromBuffer(buffer.GetReader(), mapFolder);
}

// Game Config

void InGame::LoadGameOptions()
{
    gameOptions.sensitivity = std::max(0.1f, std::stof(client_->GetConfigOption("Sensitivity", std::to_string(camController_.rotMod))));
    gameOptions.dynamicSensitivity = std::stoi(client_->GetConfigOption("DynamicSensitivity", "1"));

    gameOptions.audioEnabled = std::atoi(client_->GetConfigOption("audioEnabled", "1").c_str());
    gameOptions.audioGeneral = std::max(0, std::min(100, std::atoi(client_->GetConfigOption("audioGeneral","100").c_str())));
    gameOptions.audioAnnouncer = std::max(0, std::min(100, std::atoi(client_->GetConfigOption("audioAnnouncer","100").c_str())));
    gameOptions.audioMusic = std::max(0, std::min(100, std::atoi(client_->GetConfigOption("audioMusic","100").c_str())));
}

void InGame::WriteGameOptions()
{
    client_->config.options["Sensitivity"] = std::to_string(gameOptions.sensitivity);
    client_->config.options["DynamicSensitivity"] = std::to_string(gameOptions.dynamicSensitivity);

    client_->config.options["audioEnabled"] = std::to_string(gameOptions.audioEnabled);
    client_->config.options["audioGeneral"] = std::to_string(gameOptions.audioGeneral);
    client_->config.options["audioAnnouncer"] = std::to_string(gameOptions.audioAnnouncer);
    client_->config.options["audioMusic"] = std::to_string(gameOptions.audioMusic);
}

void InGame::ApplyGameOptions(GameOptions options)
{
    // Update saved values
    gameOptions = options;

    // Apply changes
    camController_.rotMod = gameOptions.sensitivity;

    audioMgr->SetListenerGain((float)options.audioGeneral / 100.0f);
    auto announcerGain = (float)options.audioAnnouncer / 100.0f;
    audioMgr->SetSourceParams(announcerSource, glm::vec3{0.0f}, 0.0f, false, 1.0f, announcerGain);
    auto musicGain = (float) options.audioMusic / 100.0f;
    Audio::AudioSource::Params params;
    params.gain = musicGain;
    audioMgr->SetStreamSourceParams(soundtrackSource, params);
}
