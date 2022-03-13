#include <GameState/InGame/InGame.h>

#include <Client.h>

#include <util/Random.h>
#include <util/BBTime.h>
#include <util/File.h>

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

InGame::InGame(Client* client, std::string serverDomain, uint16_t serverPort) : 
    GameState{client}, serverDomain{serverDomain}, serverPort{serverPort}, host{ENet::HostFactory::Get()->CreateHost(1, 2)}
{
}

// Public

void InGame::Start()
{
    // Window
    SDL_SetWindowResizable(this->client_->window_, SDL_TRUE);
    client_->SetWindowSize(glm::ivec2{client_->config.window.resolutionW, client_->config.window.resolutionH});

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
        shader = GL::Shader::FromFolder(client_->config.openGL.shadersFolder, "circleVertex.glsl", "circleFrag.glsl");
        chunkShader = GL::Shader::FromFolder(client_->config.openGL.shadersFolder, "chunkVertex.glsl", "chunkFrag.glsl");
        quadShader = GL::Shader::FromFolder(client_->config.openGL.shadersFolder, "quadVertex.glsl", "quadFrag.glsl");
        textShader = GL::Shader::FromFolder(client_->config.openGL.shadersFolder, "textVertex.glsl", "textFrag.glsl");
    }
    catch(const std::runtime_error& e)
    {
        this->client_->logger->LogCritical(e.what());
        client_->quit = true;
        return;
    }

    // Textures
    try{
        flashTexture = GL::Texture::FromFolder(TEXTURES_DIR, "flash.png");
        flashTexture.Load();
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
    auto black = map_.cPalette.AddColor(glm::u8vec4{0, 0, 0, 255});
    auto white = map_.cPalette.AddColor(glm::u8vec4{255, 255, 255, 255});
    //LoadMap("./resources/maps/Alpha2.bbm");
    LoadMap("/home/defu/Projects/BlockBuster/resources/maps/Alpha2.bbm");

    // UI
    inGameGui.Start();

    // Networking
    auto serverAddress = ENet::Address::CreateByDomain(serverDomain, serverPort).value();
    host.SetOnConnectCallback([this](auto id)
    {
        this->serverId = id;
        this->client_->logger->LogInfo("Succes on connection to server");
    });
    host.SetOnRecvCallback([this](auto id, auto channel, ENet::RecvPacket ePacket)
    {
        auto* packet = (Networking::Command*) ePacket.GetData();

        if(packet->header.type == Networking::Command::Type::CLIENT_CONFIG)
        {
            auto config = packet->data.config;
            client_->logger->LogInfo("Server tick rate is " + std::to_string(config.sampleRate));
            this->serverTickRate = Util::Time::Seconds(config.sampleRate);

            this->offsetTime = serverTickRate * 0.5;
            this->predOffset = serverTickRate;
            this->playerId = config.playerId;
            this->connected = true;

            client_->logger->LogInfo("We play as player " + std::to_string(this->playerId));
        }
        else if(packet->header.type == Networking::Command::Type::PLAYER_DISCONNECTED)
        {
            Networking::Command::Server::PlayerDisconnected playerDisconnect = packet->data.playerDisconnect;
            client_->logger->LogInfo("Player with id " + std::to_string(playerDisconnect.playerId) + " disconnected");

            OnPlayerLeave(playerDisconnect.playerId);
        }
        else
        {
            // Entity Interpolation - Reset offset
            auto mostRecent = snapshotHistory.Back();
            if(mostRecent.has_value() && packet->header.tick > mostRecent->serverTick)
            {
                auto om = this->offsetTime - this->serverTickRate;
                this->offsetTime = std::min(serverTickRate, std::max(om, -serverTickRate));
                client_->logger->LogInfo("Offset millis " + std::to_string(this->offsetTime.count()));
            }

            Util::Buffer::Reader reader{ePacket.GetData(), ePacket.GetSize()};
            auto opCode = reader.Read<uint16_t>();
            auto buffer = reader.ReadAll();
            auto snapShot = std::make_unique<Networking::Packets::Server::WorldUpdate>();
            snapShot->SetBuffer(std::move(buffer));
            snapShot->Read();

            client_->logger->LogInfo("Recv server snapshot for tick: " + std::to_string(snapShot->snapShot.serverTick));

            // Update last ack
            this->lastAck = snapShot->lastCmd;
            client_->logger->LogInfo("Server ack command: " + std::to_string(this->lastAck));
            
            auto s = snapShot->snapShot;
            for(auto& [playerId, player] : s.players)
            {
                // Create player entries
                if(playerTable.find(playerId) == playerTable.end())
                {
                    OnPlayerJoin(playerId, player);
                }

                // Update dmg status
                playerTable[playerId].onDmg = player.onDmg;
                if(player.onDmg)
                    client_->logger->LogInfo("Player is on dmg " + std::to_string(playerId));
            }            

            // Store snapshot
            snapshotHistory.PushBack(s);

            // Sort by tick. This is only needed if a packet arrives late.
            snapshotHistory.Sort([](Networking::Snapshot a, Networking::Snapshot b)
            {
                return a.serverTick < b.serverTick;
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

        //HandleSDLEvents();
        if(connected)
            RecvServerSnapshots();

        while(simulationLag >= serverTickRate)
        {
            HandleSDLEvents();
            DoUpdate(serverTickRate);

            if(connected)
                UpdateNetworking();

            simulationLag -= serverTickRate;
        }

        EntityInterpolation();
        SmoothPlayerMovement();
        Render();

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

// Update

void InGame::DoUpdate(Util::Time::Seconds deltaTime)
{
    auto camMode = camController_.GetMode();
    if(camMode == CameraMode::FPS)
    {
        auto player = playerTable[playerId];
        auto camPos = player.GetFPSCamPos();
        camera_.SetPos(camPos);
    }
    else
        camController_.Update();

    fpsAvatar.Update(deltaTime);

    for(auto& [playerId, playerState] : playerModelStateTable)
    {
        playerState.idlePlayer.Update(deltaTime);
        playerState.shootPlayer.Update(deltaTime);
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
            if(e.key.keysym.sym == SDLK_r)
                fpsAvatar.PlayReloadAnimation();
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
            break;
        case SDL_MOUSEBUTTONDOWN:
            {
                for(auto [playerId, player] : playerTable)
                {
                    auto mousePos = client_->GetMousePos();
                    auto winSize = client_->GetWindowSize();
                    auto ray = Rendering::ScreenToWorldRay(camera_, mousePos, glm::vec2{winSize.x, winSize.y});
                    auto playerTransform = playerTable[playerId].GetRenderTransform();
                    auto lastMoveDir = GetLastMoveDir(playerId);
                    auto collision = Game::RayCollidesWithPlayer(ray, playerTransform.position, playerTransform.rotation.y, lastMoveDir);
                    if(collision.collides)
                        std::cout << "Collision with " << std::to_string(collision.hitboxType) << "\n";
                }
            }
            break;
        }
        
        if(!inGameGui.IsMenuOpen())
            camController_.HandleSDLEvent(e);
    }
}

// Networking

void InGame::OnPlayerJoin(Entity::ID playerId, Entity::PlayerState playerState)
{
    Entity::Player player;
    player.id = playerId;
    player.ApplyState(playerState);

    playerTable[playerId] = player;
    prevPlayerTable[playerId] = player;

    // Setup player animator
    playerModelStateTable[playerId] = PlayerModelState();
    PlayerModelState& ps = playerModelStateTable[playerId];

    ps.idlePlayer.SetClip(playerAvatar.GetIdleAnim());
    ps.idlePlayer.SetTargetFloat("yPos", &ps.armsPivot.position.y);
    ps.idlePlayer.isLooping = true;
    //ps.idlePlayer.Play();

    ps.shootPlayer.SetClip(playerAvatar.GetShootAnim());
    ps.shootPlayer.SetTargetFloat("zPos", &ps.armsPivot.position.z);
    ps.shootPlayer.SetTargetBool("left-flash", &ps.leftFlashActive);
    ps.shootPlayer.SetTargetBool("right-flash", &ps.rightFlashActive);

    ps.shootPlayer.SetTargetFloat("yPos", &ps.armsPivot.position.y);
    ps.shootPlayer.SetTargetFloat("pitch", &ps.armsPivot.rotation.x);

    // Set rotation sent by server to main player
    if(this->playerId == playerId)
    {
        auto camRot = camera_.GetRotationDeg();
        camera_.SetRotationDeg(camRot.x, playerState.rot.y + 90.0f);
        GetLogger()->LogInfo("Starting yaw " + std::to_string(playerState.rot.y));
    }
}

void InGame::OnPlayerLeave(Entity::ID playerId)
{
    playerTable.erase(playerId);
    prevPlayerTable.erase(playerId);
    playerModelStateTable.erase(playerId);
}

void InGame::RecvServerSnapshots()
{
    host.PollAllEvents();
}

void InGame::UpdateNetworking()
{
    client_->logger->LogInfo("Input: Sending player inputs");

    // Sample player input
    auto mask = inGameGui.IsMenuOpen() ? Entity::PlayerInput{false} : Entity::PlayerInput{true};
    auto input = Input::GetPlayerInputNumpad(mask);
    auto mouseState = SDL_GetMouseState(nullptr, nullptr);
    auto click = mouseState & SDL_BUTTON_RIGHT;

    // Update last cmdId. Note: Breaks prediction when removed. Should prolly be ++this->cmdId in Predict.
    this->cmdId++;

    // Prediction
    Predict(input);

    SendPlayerInput();
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
        auto oldCmd = predictionHistory_.At((historySize - 1) - i);
        auto reqId = cmdId - i;

        using InputPacket = Networking::Packets::Client::Input;
        auto inputPacket = std::make_unique<InputPacket>();
        
        inputPacket->req = oldCmd->inputReq;
        inputPacket->req.reqId = reqId;

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

    auto& player = playerTable[playerId];

    // Check prediction errors
    if(!predictionHistory_.Empty() && predictionHistory_.Front()->inputReq.reqId >= this->lastAck)
    {
        // Discard old commands
        std::optional<Prediction> prediction;
        GetLogger()->LogDebug("Prediction: ACK " + std::to_string(this->lastAck));
        do
        {
            prediction = predictionHistory_.Front();
            GetLogger()->LogDebug("Prediction: Discarding prediction for " + std::to_string(prediction->inputReq.reqId));
        } while(prediction && prediction->inputReq.reqId < lastAck);

        // Checking prediction errors
        auto lastSnapshot = snapshotHistory.Back();
        if(prediction && lastSnapshot.has_value())
        {
            auto& playerPositions = lastSnapshot->players;
            bool hasPlayerData = playerPositions.find(playerId) != playerPositions.end();
            if(hasPlayerData)
            {
                auto newState = lastSnapshot->players[playerId];
                auto pState = prediction->dest;

                // On error
                auto areSame = newState == pState;
                if(!areSame)
                {
                    client_->logger->LogError("Prediction: Error on prediction");
                    client_->logger->LogError("Prediction: Prediction Id " + std::to_string(prediction->inputReq.reqId) + " ACK " + std::to_string(this->lastAck));
                    auto diff = glm::length(pState.pos - newState.pos);
                    if(diff > 0.005f)
                        client_->logger->LogError("Prediction: D " + std::to_string(diff) + " P " + glm::to_string(pState.pos) + " S " + glm::to_string(newState.pos));

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
                        GetLogger()->LogDebug("Prediction: Predicted pos is " + glm::to_string(predict->dest.pos));

                        // Update history
                        predict->origin = origin;
                        predict->dest = realState;
                        predictionHistory_.Set(i, predict.value());
                    }

                    // Update error correction values
                    auto renderState = playerTable[playerId].ExtractState();
                    errorCorrectionDiff = renderState - realState;
                    errorCorrectionStart = Util::Time::GetTime();
                }
            }
        }
    }

    auto now = Util::Time::GetTime();

    // Get prev pos
    auto preState = playerTable[playerId].ExtractState();
    auto prevPred = predictionHistory_.Back();
    if(prevPred.has_value())
        preState = prevPred->dest;

    // Run predicted command for this simulation
    auto camRot = camera_.GetRotationDeg();
    auto predState = PredPlayerState(preState, playerInput, camRot.y, serverTickRate);
    predState.rot.x = camRot.x;
    auto fov = camera_.GetParam(Rendering::Camera::FOV);
    auto aspectRatio = camera_.GetParam(Rendering::Camera::ASPECT_RATIO);
    InputReq inputReq{cmdId, playerInput, camRot.y, camRot.x, fov, aspectRatio, GetRenderTime()};

    Prediction p{inputReq, preState, predState, now};
    predictionHistory_.PushBack(p);
    predOffset = predOffset - serverTickRate;
}

void InGame::SmoothPlayerMovement()
{
    if(playerTable.find(playerId) == playerTable.end())
        return;

    auto lastPred = predictionHistory_.Back();
    auto renderPos = playerTable[playerId].ExtractState();
    if(lastPred)
    {
        auto now = Util::Time::GetTime();
        Util::Time::Seconds elapsed = now - lastPred->time;
        predOffset += deltaTime;
        auto predPos = PredPlayerState(lastPred->origin, lastPred->inputReq.playerInput, lastPred->inputReq.camYaw, predOffset);

        // Prediction Error correction
        Util::Time::Seconds errorElapsed = now - errorCorrectionStart;
        float weight = glm::max(1.0 - (errorElapsed / ERROR_CORRECTION_DURATION), 0.0);
        auto errorCorrection = errorCorrectionDiff * weight;
        renderPos = predPos + errorCorrection;
        playerTable[playerId].ApplyState(renderPos);

        #ifdef _DEBUG
                auto dist = glm::length(errorCorrection.pos);
                if(dist > 0.005)
                    client_->logger->LogError("Error correction is " + glm::to_string(errorCorrection.pos) + " W " + std::to_string(weight) + " D " + std::to_string(dist));
        #endif
    }
}

Entity::PlayerState InGame::PredPlayerState(Entity::PlayerState a, Entity::PlayerInput playerInput, float playerYaw, Util::Time::Seconds deltaTime)
{
    auto& player = playerTable[playerId];
    
    Entity::PlayerState nextState = a;
    nextState.pos = pController.UpdatePosition(a.pos, playerYaw, playerInput, map_.GetMap(), deltaTime);
    nextState.rot.y = playerYaw;

    nextState.weaponState = pController.UpdateWeapon(a.weaponState, playerInput, deltaTime);
    if(Entity::HasShot(a.weaponState, nextState.weaponState))
        fpsAvatar.PlayShootAnimation();

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

    auto oldState = playerTable[playerId].ExtractState();
    auto newState = Entity::Interpolate(state1, state2, alpha);

    if(Entity::HasShot(oldState.weaponState, newState.weaponState))
    {
        auto& modelState = playerModelStateTable.at(playerId);
        modelState.shootPlayer.Reset();
        modelState.shootPlayer.Play();
    }

    playerTable[playerId].ApplyState(newState);
}

glm::vec3 InGame::GetLastMoveDir(Entity::ID playerId) const
{
    auto ps1 = prevPlayerTable.at(playerId).ExtractState();
    auto ps2 = playerTable.at(playerId).ExtractState();
    auto moveDir = Entity::GetLastMoveDir(ps1, ps2);

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

        // Start Debug
        {
            auto oldPos = prevPlayerTable[playerId].GetTransform().position;
            auto newPos = player.GetTransform().position;
            auto diff = newPos - oldPos;
            auto distDiff = glm::length(diff);
            if(distDiff > 0.05f)
            {
                auto expectedDiff = PLAYER_SPEED *  deltaTime.count();
                client_->logger->LogDebug("Render: Player " + std::to_string(playerId) + " Prev " + glm::to_string(oldPos)
                    + " New " + glm::to_string(newPos) + " Diff " + glm::to_string(diff));

                auto offset = glm::abs(distDiff - expectedDiff);
                if(offset > 0.008f)
                {
                    client_->logger->LogDebug("Render: Difference is bigger than expected: ");
                    client_->logger->LogDebug("Found diff " + std::to_string(distDiff) + " Expected diff " + std::to_string(expectedDiff) 
                        + " Offset " + std::to_string(offset) + " Frame interval " + std::to_string(deltaTime.count()));
                }
            }
            else
                client_->logger->LogDebug("Player didn't move this frame");
        }
        // End debug

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
        auto t = playerT.GetTransformMat();
        auto transform = view * t;
        shader.SetUniformInt("dmg", player.onDmg); // TODO: Comment this or change color when player is dmg
        playerAvatar.Draw(transform);

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

// TODO: Redundancy with Project::Load
void InGame::LoadMap(std::filesystem::path filePath)
{
    using namespace Util::File;

    std::fstream file{filePath, file.binary | file.in};
    if(!file.is_open())
    {
        client_->logger->LogError("Could not open file " + filePath.string());
        std::exit(-1);
        return;
    }

    auto magic = ReadFromFile<int>(file);
    if(magic != Game::Map::Map::magicNumber)
    {
        client_->logger->LogError("Wrong format for file " + filePath.string());
        return;
    }

    // Load map
    auto bufferSize = ReadFromFile<uint32_t>(file);
    Util::Buffer buffer = ReadFromFile(file, bufferSize);
    map_ = App::Client::Map::FromBuffer(buffer.GetReader());
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
    // TODO: 
}
