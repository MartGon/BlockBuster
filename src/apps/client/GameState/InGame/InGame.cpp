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
        shader = GL::Shader::FromFolder(client_->config.openGL.shadersFolder, "simpleVertex.glsl", "simpleFrag.glsl");
        chunkShader = GL::Shader::FromFolder(client_->config.openGL.shadersFolder, "chunkVertex.glsl", "chunkFrag.glsl");
        quadShader = GL::Shader::FromFolder(client_->config.openGL.shadersFolder, "quadVertex.glsl", "quadFrag.glsl");
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

    try{
        flashTexture.LoadFromFolder(TEXTURES_DIR, "flash.png");
    }
    catch(const std::runtime_error& e)
    {
        this->client_->logger->LogError(e.what());
    }

    // Load config
    LoadGameOptions();

    // Meshes
    cylinder = Rendering::Primitive::GenerateCylinder(1.f, 1.f, 16, 1);
    sphere = Rendering::Primitive::GenerateSphere(1.0f);
    quad = Rendering::Primitive::GenerateQuad();
    cube = Rendering::Primitive::GenerateCube();
    slope = Rendering::Primitive::GenerateSlope();

    // Models
    playerAvatar.SetMeshes(quad, cube, cylinder, slope);
    playerAvatar.Start(renderMgr, shader, quadShader, flashTexture);

    fpsAvatar.SetMeshes(quad, cube, cylinder);
    fpsAvatar.Start(renderMgr, shader, quadShader, flashTexture);

    // Camera
    camera_.SetPos(glm::vec3{0, 8, 16});
    camera_.SetTarget(glm::vec3{0});
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
    //LoadMap("/home/seix/Other/Repos/BlockBuster/resources/maps/Alpha2.bbm");
    LoadMap(mapFolder, mapFileName);

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
        client_->logger->LogInfo("Could not connect to server. Quitting");
    }

    client_->quit = !connected;
    client_->logger->Flush();
}

void InGame::Update()
{
    simulationLag = serverTickRate;
    
    client_->logger->LogInfo("Update rate(s) is: " + std::to_string(serverTickRate.count()));
    while(!client_->quit)
    {
        preSimulationTime = Util::Time::GetTime();

        HandleSDLEvents();
        if(connected)
            RecvServerSnapshots();

        while(simulationLag >= serverTickRate)
        {
            //HandleSDLEvents();
            DoUpdate(serverTickRate);

            if(connected)
                UpdateNetworking();

            simulationLag -= serverTickRate;
        }
        OnNewFrame();
        Render();
        UpdateAudio();

        deltaTime = (Util::Time::GetTime() - preSimulationTime);
        simulationLag += deltaTime;
        offsetTime += deltaTime;
        client_->logger->LogInfo("Offset millis increased to " + std::to_string(offsetTime.count()));
        client_->logger->LogInfo("Update: Delta time " + std::to_string(deltaTime.count()));
        client_->logger->LogInfo("Update: Simulation lag " + std::to_string(simulationLag.count()));
    }
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
    // Update animations
    fpsAvatar.Update(deltaTime);
    for(auto& [playerId, playerState] : playerModelStateTable)
    {
        playerState.deathPlayer.Update(deltaTime);
        playerState.shootPlayer.Update(deltaTime);
    }

    // Extra data: Respawns, etc
    using namespace BlockBuster;
    respawnTimer.Update(deltaTime);
    countdownTimer.Update(deltaTime);
    if(countdownTimer.IsDone() && matchState == Match::WAITING_FOR_PLAYERS)
    {
        matchState = BlockBuster::Match::ON_GOING;
        inGameGui.countdownText.SetIsVisible(false);
    }
}

void InGame::OnNewFrame()
{
    EntityInterpolation();
    SmoothPlayerMovement();

    auto& player = GetLocalPlayer();

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
}

void InGame::HandleSDLEvents()
{
    SDL_Event e;
    
    while(SDL_PollEvent(&e) != 0)
    {
        ImGui_ImplSDL2_ProcessEvent(&e);

        switch(e.type)
        {
        case SDL_QUIT:
            client_->quit = true;
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
                inGameGui.showScoreboard = true;
            break;
        case SDL_KEYUP:
            if(e.key.keysym.sym == SDLK_TAB)
                inGameGui.showScoreboard = false;
        case SDL_MOUSEBUTTONDOWN:
            {
            }
            break;
        }
        
        if(!inGameGui.IsMenuOpen())
            camController_.HandleSDLEvent(e);
    }
}

// Handy

Entity::Player& InGame::GetLocalPlayer()
{
    return playerTable[playerId];
}

// Networking

void InGame::OnPlayerJoin(Entity::ID playerId, Networking::PlayerSnapshot playerState)
{
    Entity::Player player;
    player.id = playerId;
    player.ApplyState(playerState.ToPlayerState(player.ExtractState()));

    playerTable[playerId] = player;
    prevPlayerTable[playerId] = player;

    // Setup player animator
    playerModelStateTable[playerId] = PlayerModelState();
    PlayerModelState& ps = playerModelStateTable[playerId];

    ps.deathPlayer.SetClip(playerAvatar.GetDeathAnim());
    ps.deathPlayer.SetTargetFloat("scale", &ps.gScale);

    ps.shootPlayer.SetClip(playerAvatar.GetShootAnim());
    ps.shootPlayer.SetTargetFloat("zPos", &ps.armsPivot.position.z);
    ps.shootPlayer.SetTargetBool("left-flash", &ps.leftFlashActive);
    ps.shootPlayer.SetTargetBool("right-flash", &ps.rightFlashActive);

    ps.shootPlayer.SetTargetFloat("yPos", &ps.armsPivot.position.y);
    ps.shootPlayer.SetTargetFloat("pitch", &ps.armsPivot.rotation.x);
}

void InGame::OnPlayerLeave(Entity::ID playerId)
{
    playerTable.erase(playerId);
    prevPlayerTable.erase(playerId);
    playerModelStateTable.erase(playerId);
}

void InGame::OnConnectToServer(ENet::PeerId id)
{
    this->serverId = id;
    this->client_->logger->LogInfo("Succes on connection to server");

    // Send login packet
    auto packet = std::make_unique<Networking::Packets::Client::Login>();
    packet->playerUuid = this->playerUuid;
    packet->playerName = this->playerName;
    packet->Write();
    auto buffer = packet->GetBuffer();
    ENet::SentPacket sentPacket{buffer->GetData(), buffer->GetSize(), ENET_PACKET_FLAG_RELIABLE};
    host.SendPacket(serverId, 0, sentPacket);
}

void InGame::OnRecvPacket(ENet::PeerId id, uint8_t channelId, ENet::RecvPacket recvPacket)
{
    Util::Buffer::Reader reader{recvPacket.GetData(), recvPacket.GetSize()};
    Util::Buffer buffer = reader.ReadAll();
    
    auto packet = Networking::MakePacket<Networking::PacketType::Server>(std::move(buffer));
    if(packet)
    {
        packet->Read();
        OnRecvPacket(*packet);
    }
    else
        GetLogger()->LogError("Invalid packet recv from server");
}

void InGame::OnRecvPacket(Networking::Packet& packet)
{
    using namespace Networking::Packets::Server;

    switch (packet.GetOpcode())
    {
        case Networking::OpcodeServer::OPCODE_SERVER_BATCH:
        {
            auto batch = packet.To<Networking::Batch<Networking::PacketType::Server>>();
            for(auto i = 0; i < batch->GetPacketCount(); i++)
                OnRecvPacket(*batch->GetPacket(i));
        }
        break;

        case Networking::OpcodeServer::OPCODE_SERVER_WELCOME:
        {
            auto welcome = packet.To<Welcome>();
            client_->logger->LogInfo("Server tick rate is " + std::to_string(welcome->tickRate));

            // Set server params
            this->serverTickRate = Util::Time::Seconds(welcome->tickRate);
            this->offsetTime = serverTickRate * 0.5;
            this->playerId = welcome->playerId;
            this->connected = true;

            // Set player state
            Entity::Player player;
            player.id = this->playerId;
            GetLogger()->LogInfo("We play as player " + std::to_string(this->playerId));

            // Add to table
            playerTable[playerId] = player;
            prevPlayerTable[playerId] = player;

            // Set match data
            countdownTimer.SetDuration(welcome->timeToStart);
            countdownTimer.Start();
            matchState = welcome->matchState;
        }
        break;

        case Networking::OpcodeServer::OPCODE_SERVER_SNAPSHOT:
        {
            auto snapShot = packet.To<WorldUpdate>();
            client_->logger->LogInfo("Recv server snapshot for tick: " + std::to_string(snapShot->snapShot.serverTick));

            // Entity Interpolation - Reset offset
            auto mostRecent = snapshotHistory.Back();
            if(mostRecent.has_value() && snapShot->snapShot.serverTick > mostRecent->serverTick)
            {
                auto om = this->offsetTime - this->serverTickRate;
                this->offsetTime = std::min(serverTickRate, std::max(om, -serverTickRate));
                client_->logger->LogInfo("Offset millis " + std::to_string(this->offsetTime.count()));
            }
            
            auto s = snapShot->snapShot;
            for(auto& [playerId, player] : s.players)
            {
                // Create player entries
                if(playerTable.find(playerId) == playerTable.end() && this->playerId != playerId)
                {
                    OnPlayerJoin(playerId, player);
                }
            }            

            // Store snapshot
            snapshotHistory.PushBack(s);

            // Sort by tick. This is only needed if a packet arrives late.
            snapshotHistory.Sort([](Networking::Snapshot a, Networking::Snapshot b)
            {
                return a.serverTick < b.serverTick;
            });
        }
        break;

        case Networking::OpcodeServer::OPCODE_SERVER_PLAYER_DISCONNECTED:
        {
            auto pd = packet.To<PlayerDisconnected>();
            OnPlayerLeave(pd->playerId);
        }
        break;

        case Networking::OpcodeServer::OPCODE_SERVER_PLAYER_INPUT_ACK:
        {
            auto pi = packet.To<PlayerInputACK>();

            // Update last ack
            this->lastAck = pi->lastCmd;
            localPlayerStateHistory.PushBack(pi->playerState);
            client_->logger->LogInfo("Server ack command: " + std::to_string(this->lastAck));
            //client_->logger->LogError("Player new pos " + glm::to_string(pi->playerState.transform.pos));
        }
        break;

        case Networking::OpcodeServer::OPCODE_SERVER_PLAYER_TAKE_DMG:
        {
            auto ptd = packet.To<PlayerTakeDmg>();
            client_->logger->LogInfo("Player took dmg!");

            auto& player = GetLocalPlayer();
            player.health = ptd->healthState;

            inGameGui.PlayDmgAnim();
        }
        break;

        case Networking::OpcodeServer::OPCODE_SERVER_PLAYER_HIT_CONFIRM:
        {
            auto ptd = packet.To<PlayerHitConfirm>();
            client_->logger->LogInfo("Enemy player was hit!");

            inGameGui.PlayHitMarkerAnim(InGameGUI::HitMarkerType::DMG);
        }
        break;

        case Networking::OpcodeServer::OPCODE_SERVER_PLAYER_DIED:
        {
            auto ptd = packet.To<PlayerDied>();

            if(ptd->victimId == playerId)
            {   
                // Set player dead
                auto& player = GetLocalPlayer();
                player.health.hp = 0;

                fpsAvatar.isEnabled = false;
                inGameGui.crosshairImg.SetIsVisible(false);

                inGameGui.killText.SetIsVisible(true);
                inGameGui.respawnTimeText.SetIsVisible(true);

                // Clear prediction history. No prediction is useful
                predictionHistory_.Clear();

                respawnTimer.SetDuration(ptd->respawnTime);
                respawnTimer.Start();

                // Set killer
                killerId = ptd->killerId;
            }
            else
            {
                playerModelStateTable[ptd->victimId].deathPlayer.Reset();
                playerModelStateTable[ptd->victimId].deathPlayer.Play();
            }

            if(ptd->killerId == playerId)
                inGameGui.PlayHitMarkerAnim(InGameGUI::HitMarkerType::KILL);
        }
        break;

        case Networking::OpcodeServer::OPCODE_SERVER_PLAYER_RESPAWN:
        {
            auto respawn = packet.To<PlayerRespawn>();

            if(respawn->playerId == playerId)
            {
                localPlayerStateHistory.PushBack(respawn->playerState);
                GetLocalPlayer().ApplyState(respawn->playerState);
                GetLocalPlayer().ResetHealth();

                // Set spawn camera rotation
                auto camRot = camera_.GetRotationDeg();
                camera_.SetRotationDeg(camRot.x, respawn->playerState.transform.rot.y + 90.0f);

                // On Player respawn
                fpsAvatar.isEnabled = true;
                inGameGui.crosshairImg.SetIsVisible(true);

                inGameGui.killText.SetIsVisible(false);
                inGameGui.respawnTimeText.SetIsVisible(false);
            }
            else
            {
                // Reset scale after death animation
                playerModelStateTable[respawn->playerId].gScale = 1.0f;
                if(Util::Map::Contains(playerTable, respawn->playerId))
                    playerTable[respawn->playerId].ApplyState(respawn->playerState);
            }
        }
        break;

        default:
            break;
    }
}

void InGame::RecvServerSnapshots()
{
    host.PollAllEvents();
}

void InGame::UpdateNetworking()
{
    auto& player = GetLocalPlayer();
    bool sendInputs = !player.IsDead() && matchState == Match::ON_GOING;
    if(sendInputs)
    {
        // Sample player input
        auto mask = inGameGui.IsMenuOpen() ? Entity::PlayerInput{false} : Entity::PlayerInput{true};
        auto input = Input::GetPlayerInput(mask);

        // Prediction
        Predict(input);
        SendPlayerInput();

        // Update last cmdId
        this->cmdId++;
    }
}

void InGame::SendPlayerInput()
{
    // Build batched player input batch
    auto cmdId = this->cmdId;
    Networking::Batch<Networking::PacketType::Client> batch;

    auto historySize = predictionHistory_.GetSize();
    auto redundancy = std::min(redundantInputs, historySize);
    auto amount = redundancy + 1u;

    // Write inputs
    for(int i = 0; i < amount; i++)
    {
        auto oldCmd = predictionHistory_.At((historySize - (1 + i)));

        using InputPacket = Networking::Packets::Client::Input;
        auto inputPacket = std::make_unique<InputPacket>();
        
        inputPacket->req = oldCmd->inputReq;

        batch.PushPacket(std::move(inputPacket));
    }

    // Send them
    batch.Write();
    auto batchBuffer = batch.GetBuffer();
    ENet::SentPacket sentPacket{batchBuffer->GetData(), batchBuffer->GetSize(), 0};
    host.SendPacket(serverId, 0, sentPacket);
}

// Networking Prediction

void InGame::Predict(Entity::PlayerInput playerInput)
{
    if(playerTable.find(playerId) == playerTable.end())
        return;

    auto& player = GetLocalPlayer();

    // Check prediction errors
    if(!predictionHistory_.Empty() && predictionHistory_.Front()->inputReq.reqId <= this->lastAck)
    {
        // Discard old commands
        std::optional<Prediction> prediction;
        GetLogger()->LogDebug("Prediction: Last ACK " + std::to_string(this->lastAck));
        do
        {
            prediction = predictionHistory_.PopFront();
            GetLogger()->LogDebug("Prediction: Discarding prediction for " + std::to_string(prediction->inputReq.reqId));
        } while(prediction && prediction->inputReq.reqId < lastAck);

        // Checking prediction errors
        auto lastState = localPlayerStateHistory.Back();
        if(prediction && lastState.has_value())
        {
            auto newState = lastState.value();
            auto pState = prediction->dest;

            /*
            GetLogger()->LogError("New state pos " + glm::to_string(newState.transform.pos));
            GetLogger()->LogError("Predicted state pos " + glm::to_string(pState.transform.pos));
            GetLogger()->LogError("Render pos " + glm::to_string(playerTable[playerId].GetRenderTransform().position));
            GetLogger()->LogError("Error correction offset " + glm::to_string(errorCorrectionDiff.pos));
            */

            // On error
            auto areSame = newState == pState;
            if(!areSame)
            {
                client_->logger->LogError("Prediction: Error on prediction");
                client_->logger->LogError("Prediction: Prediction Id " + std::to_string(prediction->inputReq.reqId) + " ACK " + std::to_string(this->lastAck));
                auto diff = glm::length(pState.transform.pos - newState.transform.pos);
                if(diff > 0.005f)
                    client_->logger->LogError("Prediction: D " + std::to_string(diff) + 
                        " P " + glm::to_string(pState.transform.pos) + " S " + glm::to_string(newState.transform.pos));

                // Accept player pos
                auto realState = newState;

                // Run prev commands again
                for(auto i = 0; i < predictionHistory_.GetSize(); i++)
                {
                    auto predict = predictionHistory_.At(i);
                    GetLogger()->LogDebug("Prediction: Repeting prediction for " + std::to_string(predict->inputReq.reqId));

                    // Re-calculate prediction
                    auto origin = realState;
                    realState = PredPlayerState(origin, predict->inputReq.playerInput, predict->inputReq.camYaw, serverTickRate);
                    GetLogger()->LogDebug("Prediction: Predicted pos is " + glm::to_string(predict->dest.transform.pos));

                    // Update history
                    predict->origin = origin;
                    predict->dest = realState;
                    predictionHistory_.Set(i, predict.value());
                }

                // Update error correction values
                auto renderState = GetLocalPlayer().ExtractState();
                errorCorrectionDiff = renderState.transform - realState.transform;
                errorCorrectionStart = Util::Time::GetTime();
            }
        }
    }

    // Get prev pos
    auto preState = localPlayerStateHistory.Back().value();
    auto prevPred = predictionHistory_.Back();
    if(prevPred.has_value())
        preState = prevPred->dest;

    // Run predicted command for this simulation
    auto camRot = camera_.GetRotationDeg();
    auto predState = PredPlayerState(preState, playerInput, camRot.y, serverTickRate);
    predState.transform.rot.x = camRot.x;
    auto fov = camera_.GetParam(Rendering::Camera::FOV);
    auto aspectRatio = camera_.GetParam(Rendering::Camera::ASPECT_RATIO);
    InputReq inputReq{cmdId, playerInput, camRot.y, camRot.x, fov, aspectRatio, GetRenderTime()};

    auto now = Util::Time::GetTime();
    Prediction p{inputReq, preState, predState, now};
    predictionHistory_.PushBack(p);
}

void InGame::SmoothPlayerMovement()
{
    if(playerTable.find(playerId) == playerTable.end())
        return;

    auto& player = GetLocalPlayer();

    if(player.IsDead())
        return;

    auto lastPred = predictionHistory_.Back();
    if(lastPred)
    {
        auto now = Util::Time::GetTime();
        Util::Time::Seconds elapsed = now - lastPred->time;
        auto predState = PredPlayerState(lastPred->origin, lastPred->inputReq.playerInput, lastPred->inputReq.camYaw, elapsed);

        // Prediction Error correction
        Util::Time::Seconds errorElapsed = now - errorCorrectionStart;
        float weight = glm::max(1.0 - (errorElapsed / ERROR_CORRECTION_DURATION), 0.0);
        auto errorCorrection = errorCorrectionDiff * weight;
        predState.transform = predState.transform + errorCorrection;
        player.ApplyState(predState);

        #ifdef _DEBUG
            auto dist = glm::length(errorCorrection.pos);
            if(dist > 0.005)
                client_->logger->LogError("Error correction is " + glm::to_string(errorCorrection.pos) + " W " + std::to_string(weight) + " D " + std::to_string(dist));
        #endif
    }
}

Entity::PlayerState InGame::PredPlayerState(Entity::PlayerState a, Entity::PlayerInput playerInput, float playerYaw, Util::Time::Seconds deltaTime)
{
    auto& player = GetLocalPlayer();
    
    Entity::PlayerState nextState = a;
    nextState.transform.pos = pController.UpdatePosition(a.transform.pos, playerYaw, playerInput, map_.GetMap(), deltaTime);
    nextState.transform.rot.y = playerYaw;

    nextState.weaponState = pController.UpdateWeapon(a.weaponState, playerInput, deltaTime);

    // Animation
    if(Entity::HasShot(a.weaponState.state, nextState.weaponState.state))
        fpsAvatar.PlayShootAnimation();
    else if(Entity::HasReloaded(a.weaponState.state, nextState.weaponState.state))
        fpsAvatar.PlayReloadAnimation(nextState.weaponState.cooldown);

    return nextState;
}

// Networking Entity Interpolation

Util::Time::Seconds InGame::TickToTime(uint32_t tick)
{
    return tick * this->serverTickRate;
}

Util::Time::Seconds InGame::GetCurrentTime()
{
    auto maxTick = snapshotHistory.Back()->serverTick;
    auto base = TickToTime(maxTick);

    return base + this->offsetTime;
}

Util::Time::Seconds InGame::GetRenderTime()
{
    auto clientTime = GetCurrentTime();
    auto windowSize = 2.0 * this->serverTickRate;
    auto renderTime = clientTime - windowSize;
    
    return renderTime;
}

void InGame::EntityInterpolation()
{
    if(snapshotHistory.GetSize() < 2)
        return;

    // Calculate render time
    auto clientTime = GetCurrentTime();
    auto renderTime = GetRenderTime();

    // Get last snapshot before renderTime
    auto s1p = snapshotHistory.FindRevFirstPair([this, renderTime](auto i, auto s)
    {
        auto time = TickToTime(s.serverTick);
        return time < renderTime;
    });
    
    if(s1p.has_value())
    {
        auto s2o = snapshotHistory.At(s1p->first + 1);
        if(s2o.has_value())
        {
            // Find samples to use for interpolation
            // Sample 1: Last one before current render time
            // Sample 2: First one after current render time
            auto s1 = s1p->second;
            auto s2 = s2o.value();

            // Find weights
            auto t1 = TickToTime(s1.serverTick);
            auto t2 = TickToTime(s2.serverTick);
            auto ws = Math::GetWeights(t1.count(), t2.count(), renderTime.count());
            auto w1 = ws.x; auto w2 = ws.y;

            client_->logger->LogDebug("Tick 1 " + std::to_string(s1.serverTick) + " Tick 2 " + std::to_string(s2.serverTick));
            client_->logger->LogDebug("RT " + std::to_string(renderTime.count()) + " CT " + std::to_string(clientTime.count()) + " OT " + std::to_string(offsetTime.count()));
            client_->logger->LogDebug("T1 " + std::to_string(t1.count()) + " T2 " + std::to_string(t2.count()));
            client_->logger->LogDebug("W1 " + std::to_string(w1) + " W2 " + std::to_string(w2));

            for(auto pair : playerTable)
            { 
                auto playerId = pair.first;
                if(playerId == this->playerId)
                    continue;

                bool s1HasData = s1.players.find(pair.first) != s1.players.end();
                bool s2HasData = s2.players.find(pair.first) != s2.players.end();
                bool canInterpolate = s1HasData && s2HasData;

                if(canInterpolate)
                    EntityInterpolation(playerId, s1, s2, w1);
            }
        }
        // We don't have enough data. We need to extrapolate
        else
        {
            // Get prev snapshot with player data
            auto prev = snapshotHistory.At(s1p->first - 1);

            if(prev.has_value())
            {
                auto s0 = prev.value();
                auto s1 = s1p.value().second;

                Networking::Snapshot exS;
                exS.serverTick = s1.serverTick + 1;

                for(auto pair : playerTable)
                {
                    auto playerId = pair.first;
                    if(playerId == this->playerId)
                        continue;

                    // Extrapolate player pos
                    /*
                    auto s0Pos = s0.players[playerId].pos;
                    auto s1Pos = s1.players[playerId].pos;
                    auto offset = s1Pos - s0Pos;
                    auto s2Pos = PredPlayerState(s1Pos, offset, EXTRAPOLATION_DURATION);
                    exS.players[playerId].pos = s2Pos;

                    // Interpolate
                    auto t1 = TickToTime(s1.serverTick);
                    auto t2 = TickToTime(s1.serverTick) + EXTRAPOLATION_DURATION;
                    auto ws = Math::GetWeights(t1.count(), t2.count(), renderTime.count());
                    auto alpha = ws.x;
                    EntityInterpolation(playerId, s1, exS, alpha);
                    */
                }
            }
        }
    }
}

void InGame::EntityInterpolation(Entity::ID playerId, const Networking::Snapshot& s1, const Networking::Snapshot& s2, float alpha)
{
    auto state1 = s1.players.at(playerId);
    auto state2 = s2.players.at(playerId);

    auto& player = playerTable[playerId];
    auto oldState = player.ExtractState();
    auto interpolation = Networking::PlayerSnapshot::Interpolate(state1, state2, alpha);

    if(Entity::HasShot(oldState.weaponState.state, interpolation.wepState))
    {
        auto& modelState = playerModelStateTable.at(playerId);
        modelState.shootPlayer.SetClip(playerAvatar.GetShootAnim());
        modelState.shootPlayer.Reset();
        modelState.shootPlayer.Play();
    }

    if(Entity::HasReloaded(oldState.weaponState.state, interpolation.wepState))
    {
        auto& modelState = playerModelStateTable.at(playerId);
        modelState.shootPlayer.SetClip(playerAvatar.GetReloadAnim());
        modelState.shootPlayer.Reset();
        modelState.shootPlayer.Play();
    }

    auto newState = interpolation.ToPlayerState(oldState);
    player.ApplyState(newState);
}

glm::vec3 InGame::GetLastMoveDir(Entity::ID playerId) const
{
    auto ps1 = prevPlayerTable.at(playerId).ExtractState();
    auto ps2 = playerTable.at(playerId).ExtractState();
    auto moveDir = Entity::GetLastMoveDir(ps1.transform.pos, ps2.transform.pos);

    return moveDir;
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

        // Draw model
        playerT = player.GetRenderTransform();
        playerT.scale *= playerState.gScale;
        auto t = playerT.GetTransformMat();
        auto transform = view * t;
        playerAvatar.Draw(transform);

        // TODO: Remove
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
    }
    prevPlayerTable = playerTable;

    // Draw skybox
    skybox.Draw(skyboxShader, camera_.GetViewMat(), camera_.GetProjMat());

    // Draw models
    renderMgr.Render(camera_);

    // Draw fpsModel, always rendered last
    auto proj = camera_.GetProjMat();
    shader.SetUniformInt("dmg", 0);
    fpsAvatar.Draw(proj);
}

void InGame::DrawCollisionBox(const glm::mat4& viewProjMat, Math::Transform box)
{
    auto mat = viewProjMat * box.GetTransformMat();
    shader.SetUniformMat4("transform", mat);
    cube.Draw(shader, glm::vec4{1.0f}, GL_LINE);
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
