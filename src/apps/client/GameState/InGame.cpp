#include <GameState/InGame.h>

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

#include <iostream>
#include <algorithm>

using namespace BlockBuster;
using namespace ::App::Client;

InGame::InGame(Client* client) : GameState{client}, host{ENet::HostFactory::Get()->CreateHost(1, 2)}
{
}

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

    // Init font
    std::filesystem::path fontPath = std::filesystem::path{RESOURCES_DIR} / "fonts/Pixel.ttf";
    pixelFont = GUI::TextFactory::Get()->LoadFont(fontPath);
    text = pixelFont->CreateText();
    text.SetText("See ya");

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
    camController_.SetMode(::App::Client::CameraMode::FPS);

    // Map
    auto black = map_.cPalette.AddColor(glm::u8vec4{0, 0, 0, 255});
    auto white = map_.cPalette.AddColor(glm::u8vec4{255, 255, 255, 255});
    LoadMap("resources/maps/Alpha2.bbm");

    // Networking
    auto serverAddress = ENet::Address::CreateByIPAddress("127.0.0.1", 8081).value();
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

    // Connect to server
    auto attempts = 0;
    while(!connected && attempts < 5)
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
    auto input = Input::GetPlayerInputNumpad();
    auto mouseState = SDL_GetMouseState(nullptr, nullptr);
    auto click = mouseState & SDL_BUTTON_RIGHT;

    // Update last cmdId. Note: Breaks prediction when removed. Should prolly be ++this->cmdId in Predict.
    this->cmdId++;

    // Prediction
    Predict(input);

    SendPlayerInput();

    // Player shots
    if(click)
    {
        client_->logger->LogInfo("Player is clicking");
        auto ray = Rendering::ScreenToWorldRay(camera_, client_->GetMousePos(), client_->GetWindowSize());
        client_->logger->LogInfo("Shooting from " + glm::to_string(ray.origin) + " to " + glm::to_string(ray.dest));

        for(auto [id, player] : playerTable)
        {
            auto pt = player.GetRenderTransform();
            auto collision = Collisions::RayAABBIntersection(ray, pt.GetTransformMat());
            if(collision.intersects)
            {
                client_->logger->LogInfo("It collides with player " + std::to_string(id));
                client_->logger->LogInfo("Player was at " + glm::to_string(pt.position));
                client_->logger->LogInfo("Command time is " + std::to_string(GetRenderTime().count()));
            }
        }

        Networking::Command::Header header;
        header.type = Networking::Command::PLAYER_SHOT;

        Networking::Command::User::PlayerShot playerShot;
        playerShot.origin = ray.origin;
        playerShot.dir = ray.dest;
        playerShot.clientTime = GetRenderTime().count();

        Networking::Command::Payload payload;
        payload.playerShot = playerShot;

        Networking::Command cmd{header, payload};

        ENet::SentPacket sentPacket{&cmd, sizeof(cmd), 0};
        //host.SendPacket(serverId, 0, sentPacket);
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

void InGame::Predict(Entity::PlayerInput playerInput)
{
    if(playerTable.find(playerId) == playerTable.end())
        return;

    auto& player = playerTable[playerId];

    // Discard old commands
    std::optional<Prediction> prediction = predictionHistory_.Front();
    while(prediction && prediction->inputReq.reqId < lastAck)
    {
        prediction = predictionHistory_.PopFront();
    }
    
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
            auto diff = Entity::GetDifference(newState, pState);

            // On error
            if(diff >= 0.005)
            {
                client_->logger->LogError("Prediction: Error on prediction");
                client_->logger->LogError("Prediction: Tick " + std::to_string(lastSnapshot->serverTick) + " ACK " + std::to_string(this->lastAck));
                client_->logger->LogError("Prediction: D " + std::to_string(diff) + " P " + glm::to_string(pState.pos) + " S " + glm::to_string(newState.pos));

                // Accept player pos
                auto realState = newState;

                // Run prev commands again
                for(auto i = 0; i < predictionHistory_.GetSize(); i++)
                {
                    auto predict = predictionHistory_.At(i);

                    // Re-calculate prediction
                    auto origin = realState;
                    realState = PredPlayerState(realState, predict->inputReq.playerInput, predict->inputReq.camYaw, serverTickRate);

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
    Prediction p{InputReq{cmdId, playerInput, camRot.y, camRot.x}, preState, predState, now};
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
    }
}

Entity::PlayerState InGame::PredPlayerState(Entity::PlayerState a, Entity::PlayerInput playerInput, float playerYaw, Util::Time::Seconds deltaTime)
{
    auto& player = playerTable[playerId];
    
    a.pos = pController.UpdatePosition(a.pos, playerYaw, playerInput, map_.GetMap(), deltaTime);
    a.rot.y = playerYaw;

    return a;
}

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

    auto smoothPos = Entity::Interpolate(state1, state2, alpha);
    playerTable[playerId].ApplyState(smoothPos);
}

glm::vec3 InGame::GetLastMoveDir(Entity::ID playerId) const
{
    auto pt1 = prevPlayerTable.at(playerId).GetTransform();
    auto pt2 = playerTable.at(playerId).GetTransform();
    auto moveDir = pt1.position - pt2.position;

    auto len = glm::length(moveDir);
    moveDir = len > 0.005f ? moveDir / len : glm::vec3{0.0f};

    return moveDir;
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
            // TODO: Remove before release
            if(e.key.keysym.sym == SDLK_ESCAPE)
                client_->quit = true;
            break;
        }
        
        camController_.HandleSDLEvent(e);
    }
}

void InGame::Render()
{
    // Clear Buffer
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    DrawScene();
    DrawGUI();

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
        playerAvatar.SetFacing(facingAngle);
        auto cPitch = std::min(std::max(playerT.rotation.x, 60.0f), 120.0f);
        auto pitch = -(cPitch- 90.0f);
        playerAvatar.RotateArms(pitch);
        playerAvatar.SteerWheels(GetLastMoveDir(playerId), playerT.rotation.y);

        // Draw model
        playerT = player.GetRenderTransform();
        auto t = playerT.GetTransformMat();
        auto transform = view * t;
        shader.SetUniformInt("dmg", player.onDmg);
        playerAvatar.Draw(transform);

        // Draw player hitbox
        // TODO: Comment this block. Convert into a debugging function
        Math::Transform boxTf = pController.GetECB();
        auto mat = view * boxTf.GetTransformMat();
        shader.SetUniformMat4("transform", mat);
        cube.Draw(shader, glm::vec4{1.0f}, GL_LINE);
    }
    prevPlayerTable = playerTable;

    // Draw fpsModel, always rendered
    auto proj = camera_.GetProjMat();
    fpsAvatar.Draw(proj);

    renderMgr.Render(camera_);
}

void InGame::DrawGUI()
{
    // Clear GUI buffer
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(client_->window_);
    ImGui::NewFrame();

    bool isOpen = true;
    if(connected)
    {
        if(ImGui::Begin("Network Stats"))
        {
            auto info = host.GetPeerInfo(serverId);

            ImGui::Text("Config");
            ImGui::Separator();
            ImGui::Text("Server tick rate: %.2f ms", serverTickRate.count());

            ImGui::Text("Latency");
            ImGui::Separator();
            ImGui::Text("Ping:");ImGui::SameLine();ImGui::Text("%i ms", info.roundTripTimeMs);
            ImGui::Text("Ping variance: %i", info.roundTripTimeVariance);

            ImGui::Text("Packets");
            ImGui::Separator();
            ImGui::Text("Packets sent: %i", info.packetsSent);
            ImGui::Text("Packets ack: %i", info.packetsAck);
            ImGui::Text("Packets lost: %i", info.packetsLost);
            ImGui::Text("Packet loss: %i", info.packetLoss);

            ImGui::Text("Bandwidth");
            ImGui::Separator();
            ImGui::Text("In: %i B/s", info.incomingBandwidth);
            ImGui::Text("Out: %i B/s", info.outgoingBandwidth);
        }
        ImGui::End();

        if(ImGui::Begin("Rendering Stats"))
        {
            ImGui::Text("Latency");
            ImGui::Separator();
            uint64_t frameIntervalMs = deltaTime.count() * 1e3;
            ImGui::Text("Frame interval (delta time):");ImGui::SameLine();ImGui::Text("%lu ms", frameIntervalMs);
            uint64_t fps = 1.0 / deltaTime.count();
            ImGui::Text("FPS: %lu", fps);

            ImGui::InputDouble("Max FPS", &maxFPS, 1.0, 5.0, "%.0f");
            minFrameInterval = Util::Time::Seconds(1.0 / maxFPS);
            ImGui::Text("Min Frame interval:");ImGui::SameLine();ImGui::Text("%f ms", minFrameInterval.count());
        }
        ImGui::End();

        if(ImGui::Begin("Transform"))
        {
            if(ImGui::InputInt("ID", (int*)&playerId))
            {
                if(auto sm = playerAvatar.armsModel->GetSubModel(modelId))
                {
                    modelOffset = playerModelStateTable[playerId].armsPivot.position;
                    modelScale =  playerAvatar.aTransform.scale;
                    modelRot =  playerAvatar.aTransform.rotation;
                }
            }
            ImGui::SliderFloat3("Offset", &playerModelStateTable[playerId].armsPivot.position.x, -sliderPrecision, sliderPrecision);
            ImGui::SliderFloat3("Scale", &fpsAvatar.idlePivot.scale.x, -sliderPrecision, sliderPrecision);
            ImGui::SliderFloat3("Rotation", &fpsAvatar.idlePivot.rotation.x, -sliderPrecision, sliderPrecision);
            ImGui::InputFloat("Precision", &sliderPrecision);
            ImGui::InputFloat("Facing Angle", &facingAngle);
            if(ImGui::Button("Apply"))
            {
                // Edit player model
                if(auto sm = playerAvatar.armsModel->GetSubModel(modelId))
                {
                    playerAvatar.aTransform.position = modelOffset;
                    playerAvatar.aTransform.scale = modelScale;
                    playerAvatar.aTransform.rotation = modelRot;
                }
            }
            if(ImGui::Button("Shoot"))
            {
                fpsAvatar.PlayShootAnimation();
                for(auto& [playerId, playerState] : playerModelStateTable)
                {
                    playerState.shootPlayer.Reset();
                    playerState.shootPlayer.Play();
                    break;
                }
            }
        }
        ImGui::End();
    }
    
    // Draw GUI
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData(), guiVao.GetHandle());
    auto windowSize = client_->GetWindowSize();
    int scissor_box[4] = { 0, 0, windowSize.x, windowSize.y };
    ImGui_ImplOpenGL3_RestoreState(scissor_box);

    text.SetColor(glm::vec4{0.0f, 1.0f, 0.0f, 1.0f});
    text.SetScale(2.0f);
    text.Draw(textShader, glm::vec2{0.0f, 0.0f}, glm::vec2{(float)windowSize.x, (float)windowSize.y});
}

// TODO: Redundancy with Project::Load
void InGame::LoadMap(std::filesystem::path filePath)
{
    using namespace Util::File;

    std::fstream file{filePath, file.binary | file.in};
    if(!file.is_open())
    {
        client_->logger->LogError("Could not open file " + filePath.string());
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