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
    Rendering::ColorU8ToFloat(105, 105, 105), // GREY
};

glm::vec4 InGame::ffaColors[16] = {
    glm::vec4{0.065f, 0.072f, 0.8f, 1.0f}, // BLUE
    glm::vec4{0.8, 0.072f, 0.065f, 1.0f},  // RED
    Rendering::ColorU8ToFloat(0, 100, 0), // DARK GREEN
    Rendering::ColorU8ToFloat(255, 215, 0), // GOLD

    Rendering::ColorU8ToFloat(20, 20, 20), // BLACK
    Rendering::ColorU8ToFloat(192,192,192), // WHITE
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
        textShader = GL::Shader::FromFolder(client_->config.openGL.shadersFolder, "textVertex.glsl", "textFrag.glsl");
        imgShader = GL::Shader::FromFolder(client_->config.openGL.shadersFolder, "imgVertex.glsl", "imgFrag.glsl");
        skyboxShader = GL::Shader::FromFolder(client_->config.openGL.shadersFolder, "skyboxVertex.glsl", "skyboxFrag.glsl");
    }
    catch(const std::runtime_error& e)
    {
        this->client_->logger->LogCritical(e.what());
        client_->quit = true;
        return;
    }

    // Textures
    renderMgr.Start();
    std::filesystem::path texturesDir = TEXTURES_DIR;
    GL::Cubemap::TextureMap map = {
        {GL::Cubemap::RIGHT, texturesDir / "right.jpg"},
        {GL::Cubemap::LEFT, texturesDir / "left.jpg"},
        {GL::Cubemap::TOP, texturesDir / "top.jpg"},
        {GL::Cubemap::BOTTOM, texturesDir / "bottom.jpg"},
        {GL::Cubemap::FRONT, texturesDir / "front.jpg"},
        {GL::Cubemap::BACK, texturesDir / "back.jpg"},
    };
    TRY_LOAD(skybox.Load(map, false));

    auto& texMgr = renderMgr.GetTextureMgr();
    texMgr.SetDefaultFolder(texturesDir);
    flagIconId = texMgr.LoadFromDefaultFolder("flagIcon.png", true);

    // Meshes
    cylinder = Rendering::Primitive::GenerateCylinder(1.f, 1.f, 16, 1);
    sphere = Rendering::Primitive::GenerateSphere(1.0f);
    quad = Rendering::Primitive::GenerateQuad();
    cube = Rendering::Primitive::GenerateCube();
    slope = Rendering::Primitive::GenerateSlope();

    // Models
    modelMgr.Start(renderMgr, renderShader);
    playerAvatar.SetMeshes(quad, cube, cylinder, slope);
    playerAvatar.Start(renderMgr, renderShader, renderShader);

    fpsAvatar.SetMeshes(quad, cube, cylinder);
    fpsAvatar.Start(renderMgr, renderShader, renderShader);

    // Billboards
    flagIcon = renderMgr.CreateBillboard();
    flagIcon->shader = &billboardShader;
    flagIcon->painting.type = Rendering::PaintingType::TEXTURE;
    flagIcon->painting.hasAlpha = true;
    flagIcon->painting.texture = flagIconId;

    // Camera
    auto winSize = client_->GetWindowSize();
    camera_.SetParam(Rendering::Camera::Param::ASPECT_RATIO, (float)winSize.x / (float)winSize.y);
    camera_.SetParam(Rendering::Camera::Param::FOV, client_->config.window.fov);
    camController_ = ::App::Client::CameraController{&camera_, {client_->window_, client_->io_}, ::App::Client::CameraMode::EDITOR};
    auto sensitivity = std::max(0.1f, std::stof(client_->GetConfigOption("Sensitivity", std::to_string(camController_.rotMod))));
    camController_.rotMod = sensitivity;
    //camController_.SetMode(::App::Client::CameraMode::FPS);

    // Map
    auto mapFolder = client_->mapMgr.GetMapFolder(mapName);
    auto mapFileName = mapName + ".bbm";
    LoadMap(mapFolder, mapFileName);

    // Match
    match.SetOnEnterState([this](Match::StateType type){
        this->OnEnterMatchState(type);
    });

    // UI
    inGameGui.Start();

    // Audio
    audioMgr = audioMgr->Get();
    audioMgr->Init();

    std::filesystem::path soundTrackPath = "/home/defu/Projects/BlockBuster/resources/audio/Soundtrack.wav";
    auto [id, err] = audioMgr->LoadStreamedWAVOrNull(soundTrackPath);
    if(err != Audio::AudioMgr::LoadWAVError::NO_ERR)
        GetLogger()->LogError("Could not find audio file" + soundTrackPath.string() + ". Err: " + std::to_string(err));

    auto srcId = audioMgr->CreateStreamSource();
    Audio::AudioSource::Params audioParams;
    audioMgr->SetStreamSourceParams(srcId, audioParams);
    audioMgr->SetStreamSourceAudio(srcId, id);
    audioMgr->PlayStreamSource(srcId);

    // Positional audio needs to be mono. Stereo is pre-computed.
    std::filesystem::path audioPath = "/home/defu/Projects/BlockBuster/resources/audio/tone.wav";
    auto [staticId, error] = audioMgr->LoadStaticWAVOrNull(audioPath);

    auto sSource = audioMgr->CreateSource();
    audioMgr->SetSourceAudio(sSource, staticId);

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

    client_->logger->LogInfo("Connecting to server at " + serverAddress.GetHostIP() + ":" + std::to_string(serverAddress.GetPort()));
    // Connect to server
    auto attempts = 0;
    while(!connected && attempts < 10)
    {
        Util::Time::Sleep(Util::Time::Millis{500});
        client_->logger->LogInfo("Connecting to server...");
        host.PollAllEvents();
        attempts++;
    }

    if(!connected)
    {
        client_->logger->LogError("Could not connect to server. Quitting");
    }

    exit = !connected;
    client_->logger->Flush();
}

void InGame::Update()
{
    simulationLag = serverTickRate;
    
    client_->logger->LogInfo("Update rate(s) is: " + std::to_string(serverTickRate.count()));
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
        client_->logger->LogInfo("Offset millis increased to " + std::to_string(offsetTime.count()));
        client_->logger->LogInfo("Update: Delta time " + std::to_string(deltaTime.count()));
        client_->logger->LogInfo("Update: Simulation lag " + std::to_string(simulationLag.count()));
    }

    ReturnToMainMenu();
}

void InGame::Shutdown()
{
    WriteGameOptions();
}

void InGame::ApplyVideoOptions(App::Configuration::WindowConfig& winConfig)
{
    auto winSize = client_->GetWindowSize();
    camera_.SetParam(camera_.ASPECT_RATIO, (float) winSize.x / (float) winSize.y);
    camera_.SetParam(Rendering::Camera::Param::FOV, winConfig.fov);
}

// Update

void InGame::DoUpdate(Util::Time::Seconds deltaTime)
{
    // Extra data: Respawns, etc
    respawnTimer.Update(deltaTime);
    match.Update(GetWorld(), deltaTime);
    if(match.GetState() == Match::ON_GOING)
        UpdateGameMode();
}

void InGame::OnEnterMatchState(Match::StateType type)
{
    switch (type)
    {
    case Match::StateType::ON_GOING:
        inGameGui.countdownText.SetIsVisible(false);
        inGameGui.gameTimeText.SetIsVisible(true);
        inGameGui.EnableScore();
        break;
    
    case Match::StateType::ENDING:
        inGameGui.EnableWinnerText(true);
        predictionHistory_.Clear();
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

    // Set camera in player
    auto camMode = camController_.GetMode();
    if(camMode == CameraMode::FPS)
    {
        auto camPos = player.GetFPSCamPos();
        camera_.SetPos(camPos);
    }
    else
        camController_.Update();

    // Rellocate camera
    if(player.IsDead() && Util::Map::Contains(playerTable, killerId))
    {
        // Look to killer during death
        auto& killer = playerTable[killerId];
        auto pos = killer.GetRenderTransform().position;
        camera_.SetTarget(pos);
    }

    // Update animations
    fpsAvatar.Update(deltaTime);
    for(auto& [playerId, playerState] : playerModelStateTable)
    {
        playerState.deathPlayer.Update(deltaTime);
        playerState.shootPlayer.Update(deltaTime);
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
            break;
        case SDL_KEYDOWN:
            if(e.key.keysym.sym == SDLK_f)
                drawMode = drawMode == GL_FILL ? GL_LINE : GL_FILL;
            if(e.key.keysym.sym == SDLK_p)
            {
                using namespace ::App::Client;
                auto mode = this->camController_.GetMode();
                auto nextMode = mode == CameraMode::EDITOR ? CameraMode::FPS : CameraMode::EDITOR;
                this->camController_.SetMode(nextMode);
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

// Handy

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
    skybox.Draw(skyboxShader, camera_.GetViewMat(), camera_.GetProjMat());

    // Draw models
    DrawGameObjects();
    DrawModeObjects();
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
                auto renderflags = Rendering::RenderMgr::RenderFlags::NO_FACE_CULLING | Rendering::RenderMgr::RenderFlags::IGNORE_DEPTH;
                flagIcon->Draw(view, iconPos, camera_.GetRight(), camera_.GetUp(), glm::vec2{2.f}, color, renderflags);
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
                    auto renderflags = Rendering::RenderMgr::RenderFlags::NO_FACE_CULLING | Rendering::RenderMgr::RenderFlags::IGNORE_DEPTH;
                    flagIcon->Draw(view, iconPos, camera_.GetRight(), camera_.GetUp(), glm::vec2{2.f}, color, renderflags);
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

void InGame::DrawCollisionBox(const glm::mat4& viewProjMat, Math::Transform box)
{
    auto mat = viewProjMat * box.GetTransformMat();
    renderShader.SetUniformMat4("transform", mat);
    cube.Draw(renderShader, glm::vec4{1.0f}, GL_LINE);
}

// Audio

void InGame::UpdateAudio()
{
    audioMgr->SetListenerParams(camera_.GetPos(), camera_.GetRotation().y);
    audioMgr->Update();
}

// TODO: Redundancy with Project::Load
void InGame::LoadMap(std::filesystem::path mapFolder, std::string fileName)
{
    using namespace Util::File;
    auto filepath = mapFolder / fileName;
    std::fstream file{filepath, file.binary | file.in};
    if(!file.is_open())
    {
        client_->logger->LogError("Could not open file " + filepath.string());
        std::exit(-1);
        return;
    }

    auto magic = ReadFromFile<int>(file);
    if(magic != Game::Map::Map::magicNumber)
    {
        client_->logger->LogError("Wrong format for file " + filepath.string());
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
    gameOptions.sensitivity= std::max(0.1f, std::stof(client_->GetConfigOption("Sensitivity", std::to_string(camController_.rotMod))));
    gameOptions.audioEnabled = std::atoi(client_->GetConfigOption("audioEnabled", "1").c_str());
    gameOptions.audioGeneral = std::max(0, std::min(100, std::atoi(client_->GetConfigOption("audioGeneral","100").c_str())));
}

void InGame::WriteGameOptions()
{
    client_->config.options["Sensitivity"] = std::to_string(gameOptions.sensitivity);
    client_->config.options["audioEnabled"] = std::to_string(gameOptions.audioEnabled);
    client_->config.options["audioGeneral"] = std::to_string(gameOptions.audioGeneral);
}

void InGame::ApplyGameOptions(GameOptions options)
{
    // Update saved values
    gameOptions = options;

    // Apply changes
    camController_.rotMod = gameOptions.sensitivity;
    // TODO: Apply sound
}
