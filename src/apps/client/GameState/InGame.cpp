#include <GameState/InGame.h>

#include <Client.h>

#include <util/Random.h>
#include <util/BBTime.h>

#include <math/Interpolation.h>

#include <debug/Debug.h>
#include <nlohmann/json.hpp>
#include <httplib/httplib.h>


#include <iostream>
#include <algorithm>

using namespace BlockBuster;

InGame::InGame(Client* client) : GameState{client}, host{ENet::HostFactory::Get()->CreateHost(1, 2)}
{
}

void InGame::Start()
{
    // GL features
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // Shaders
    // Load shaders
    try{
        shader = GL::Shader::FromFolder(client_->config.openGL.shadersFolder, "circleVertex.glsl", "circleFrag.glsl");
        chunkShader = GL::Shader::FromFolder(client_->config.openGL.shadersFolder, "chunkVertex.glsl", "chunkFrag.glsl");
    }
    catch(const std::runtime_error& e)
    {
        this->client_->logger->LogCritical(e.what());
        client_->quit = true;
        return;
    }

    // Meshes
    cylinder = Rendering::Primitive::GenerateCylinder(2.f, 4.f, 32, 1);
    sphere = Rendering::Primitive::GenerateSphere(1.0f);
    cube = Rendering::Primitive::GenerateCube();

    // Craft PlayerModel - Implement Blender design
    auto upCubeT = Math::Transform{glm::vec3{0.0f, 0.5f, 0.0f}, glm::vec3{0.0f}, glm::vec3{1.0f, 0.5f, 1.0f}};
    Rendering::Painting painting;
    painting.type = Rendering::PaintingType::COLOR;
    painting.color = glm::vec4{1.0f, 1.0f, 1.0f, 1.0f};
    auto sm1 = Rendering::SubModel{upCubeT, painting, Rendering::Primitive::GenerateCube(), &shader};
    playerModel.AddSubModel(std::move(sm1));

    auto downCubeT = Math::Transform{glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.0f}, glm::vec3{1.0f, 0.5f, 1.0f}};
    painting.color = glm::vec4{0.0f, 0.0f, 0.0f, 1.0f};
    auto sm2 = Rendering::SubModel{downCubeT, painting, Rendering::Primitive::GenerateCube(), &shader};
    playerModel.AddSubModel(std::move(sm2));
    

    // Camera
    camera_.SetPos(glm::vec3{0, 8, 16});
    camera_.SetTarget(glm::vec3{0});
    auto winSize = client_->GetWindowSize();
    camera_.SetParam(Rendering::Camera::Param::ASPECT_RATIO, (float)winSize.x / (float)winSize.y);
    camera_.SetParam(Rendering::Camera::Param::FOV, client_->config.window.fov);
    camController_ = ::App::Client::CameraController{&camera_, {client_->window_, client_->io_}, ::App::Client::CameraMode::EDITOR};

    // Map
    auto black = map_.cPalette.AddColor(glm::u8vec4{0, 0, 0, 255});
    auto white = map_.cPalette.AddColor(glm::u8vec4{255, 255, 255, 255});

    Game::BlockRot rot{Game::ROT_0, Game::ROT_0};
    for(int x = -8; x < 8; x++)
    {
        for(int z = -8; z < 8; z++)
        {
            auto colorId = ((x + z) & 2) ? black.data.id : white.data.id;
            Game::Display display{Game::DisplayType::COLOR, colorId};
            map_.SetBlock(glm::ivec3{x, 0, z}, Game::Block{Game::BlockType::BLOCK, rot, display});
        }
    }

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
        else if(packet->header.type == Networking::Command::Type::SERVER_SNAPSHOT)
        {
            client_->logger->LogInfo("Recv server snapshot for tick: " + std::to_string(packet->header.tick));

            // Entity Interpolation - Reset offset
            auto mostRecent = snapshotHistory.Back();
            if(mostRecent.has_value() && packet->header.tick > mostRecent->serverTick)
            {
                auto om = this->offsetTime - this->serverTickRate;
                this->offsetTime = std::min(serverTickRate, std::max(om, -serverTickRate));
                client_->logger->LogInfo("Offset millis " + std::to_string(this->offsetTime.count()));
            }

            // Process Snapshot
            auto update = packet->data.snapshot;

            // Update last ack
            this->lastAck = update.lastCmd;
            client_->logger->LogInfo("Server ack command: " + std::to_string(update.lastCmd));
            
            Util::Buffer::Reader reader{packet, ePacket.GetSize()};
            reader.Skip(sizeof(Networking::Command));
            auto s = Networking::Snapshot::FromBuffer(reader);
            for(auto& [playerId, player] : s.players)
            {
                // Create player entries
                if(playerTable.find(playerId) == playerTable.end())
                {
                    Math::Transform transform{player.pos, glm::vec3{0.0f}, glm::vec3{2.f, 4.0f, 2.f}};
                    Entity::Player player{playerId, transform};
                    playerTable[playerId] = player;
                    prevPlayerPos[playerId] = player;
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
        else if(packet->header.type == Networking::Command::Type::PLAYER_DISCONNECTED)
        {
            Networking::Command::Server::PlayerDisconnected playerDisconnect = packet->data.playerDisconnect;
            client_->logger->LogInfo("Player with id " + std::to_string(playerDisconnect.playerId) + " disconnected");

            playerTable.erase(playerDisconnect.playerId);
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

        HandleSDLEvents();
        if(connected)
            RecvServerSnapshots();

        while(simulationLag >= serverTickRate)
        {
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
    camController_.Update();
}

void InGame::RecvServerSnapshots()
{
    host.PollAllEvents();
}

void InGame::UpdateNetworking()
{
    client_->logger->LogInfo("Input: Sending player inputs");

    // Sample player input
    int8_t* state = (int8_t*)SDL_GetKeyboardState(nullptr);
    glm::vec3 moveDir{0.0f};
    moveDir.x = state[SDL_SCANCODE_KP_6] - state[SDL_SCANCODE_KP_4];
    moveDir.z = state[SDL_SCANCODE_KP_2] - state[SDL_SCANCODE_KP_8];
    auto len = glm::length(moveDir);
    moveDir = len > 0 ? moveDir / len : moveDir;

    auto mouseState = SDL_GetMouseState(nullptr, nullptr);
    auto click = mouseState & SDL_BUTTON_RIGHT;

    // Build batched player input batch
    auto cmdId = this->cmdId++;
    Networking::Command::Header header;
    header.type = Networking::Command::Type::PLAYER_MOVEMENT_BATCH;
    header.tick = cmdId;

    auto historySize = predictionHistory_.GetSize();
    auto redundancy = std::min(redundantInputs, historySize);
    auto amount = redundancy + 1u;
    Networking::Command::User::PlayerMovementBatch playerMovementBatch;
    playerMovementBatch.amount = amount;

    Networking::Command::Payload data;
    data.playerMovementBatch = playerMovementBatch;
    
    Networking::Command cmd{header, data};

    // Write them to a buffer
    Util::Buffer buf;
    buf.Write(cmd);

        // Write recent input
    Networking::Command::User::PlayerMovement playerMovement;
    playerMovement.reqId = cmdId;
    playerMovement.moveDir = moveDir;
    buf.Write(playerMovement);

        // Write redudant inputs
    for(int i = 0; i < redundancy; i++)
    {
        auto oldCmd = predictionHistory_.At((historySize - 1) - i);
        buf.Write(oldCmd->cmd);
        client_->logger->LogInfo("Sending input with reqId " + std::to_string(oldCmd->cmd.reqId));
    }

    // Send them
    ENet::SentPacket sentPacket{buf.GetData(), buf.GetSize(), 0};
    host.SendPacket(serverId, 0, sentPacket);

    // Prediction
    PredictPlayerMovement(playerMovement, cmdId);

    // Player shots
    if(click)
    {
        client_->logger->LogInfo("Player is clicking");
        auto ray = Rendering::ScreenToWorldRay(camera_, client_->GetMousePos(), client_->GetWindowSize());
        client_->logger->LogInfo("Shooting from " + glm::to_string(ray.origin) + " to " + glm::to_string(ray.dest));

        for(auto [id, player] : playerTable)
        {
            auto collision = Collisions::RayAABBIntersection(ray, player.transform.GetTransformMat());
            if(collision.intersects)
            {
                client_->logger->LogInfo("It collides with player " + std::to_string(id));
                client_->logger->LogInfo("Player was at " + glm::to_string(player.transform.position));
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
        host.SendPacket(serverId, 0, sentPacket);
    }
}

void InGame::PredictPlayerMovement(Networking::Command::User::PlayerMovement cmd, uint32_t cmdId)
{
    if(playerTable.find(playerId) == playerTable.end())
        return;

    auto& player = playerTable[playerId];

    // Discard old commands
    std::optional<Prediction> prediction;
    while(predictionHistory_.GetSize() > 0 && predictionHistory_.Front()->cmd.reqId <= lastAck)
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
            auto newPos = lastSnapshot->players[playerId].pos;
            auto pPos = prediction->dest;
            auto diff = glm::length(newPos - pPos);

            // On error
            if(diff >= 0.005)
            {
                client_->logger->LogError("Prediction: Error on prediction");
                client_->logger->LogError("Prediction: Tick " + std::to_string(lastSnapshot->serverTick) + " ACK " + std::to_string(this->lastAck));
                client_->logger->LogError("Prediction: D " + std::to_string(diff) + " P " + glm::to_string(pPos) + " S " + glm::to_string(newPos));

                // Accept player pos
                auto realPos = newPos;

                // Run prev commands again
                for(auto i = 0; i < predictionHistory_.GetSize(); i++)
                {
                    auto predict = predictionHistory_.At(i);

                    // Re-calculate prediction
                    auto move = predict->cmd;
                    auto origin = realPos;
                    realPos = PredPlayerPos(realPos, move.moveDir, serverTickRate);

                    // Update history
                    predict->origin = origin;
                    predict->dest = realPos;
                    predictionHistory_.Set(i, predict.value());
                }

                // Update error correction values
                auto renderPos = playerTable[playerId].transform.position;
                errorCorrectionDiff = renderPos - realPos;
                errorCorrectionStart = Util::Time::GetTime();
            }
        }
    }

    auto now = Util::Time::GetTime();

    // Get prev pos
    auto prevPos = playerTable[playerId].transform.position;
    auto prevPred = predictionHistory_.Back();
    if(prevPred.has_value())
    {
        prevPos = prevPred->dest;
        Util::Time::Seconds diff = now - prevPred->time;
        client_->logger->LogDebug("Prediction: Time between preds " + std::to_string(diff.count()));
    }

    // Run predicted command for this simulation
    auto predictedPos = PredPlayerPos(prevPos, cmd.moveDir, serverTickRate);
    Prediction p{cmd, prevPos, predictedPos, now};
    predictionHistory_.PushBack(p);
    predOffset = predOffset - serverTickRate;

#ifdef _DEBUG
    auto dist = glm::length(predictedPos - prevPos);
    client_->logger->LogInfo("Prediction: PrevPos " + glm::to_string(prevPos) + " PredPos " + glm::to_string(predictedPos) + " Dist " + std::to_string(dist));
#endif
}

void InGame::SmoothPlayerMovement()
{
    if(playerTable.find(playerId) == playerTable.end())
        return;

    auto lastPred = predictionHistory_.Back();
    auto& renderPos = playerTable[playerId].transform.position;
    if(lastPred)
    {
        auto now = Util::Time::GetTime();
        Util::Time::Seconds elapsed = now - lastPred->time;
        predOffset += deltaTime;
        auto predPos = PredPlayerPos(lastPred->origin, lastPred->cmd.moveDir, predOffset);

        // Prediction Error correction
        Util::Time::Seconds errorElapsed = now - errorCorrectionStart;
        float weight = glm::max(1.0 - (errorElapsed / ERROR_CORRECTION_DURATION), 0.0);
        glm::vec3 errorCorrection = errorCorrectionDiff * weight;
        renderPos = predPos + errorCorrection;

#ifdef _DEBUG
        auto dist = glm::length(errorCorrection);
        if(dist > 0.005)
            client_->logger->LogError("Error correction is " + glm::to_string(errorCorrection) + " W " + std::to_string(weight) + " D " + std::to_string(dist));
#endif
    }
}

glm::vec3 InGame::PredPlayerPos(glm::vec3 pos, glm::vec3 moveDir, Util::Time::Seconds deltaTime)
{
    auto& player = playerTable[playerId];
    auto velocity = moveDir * PLAYER_SPEED * (float)deltaTime.count();
    auto predPos = pos + velocity;
    return predPos;
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
            auto ws = Math::Interpolation::GetWeights(t1.count(), t2.count(), renderTime.count());
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
                    auto s0Pos = s0.players[playerId].pos;
                    auto s1Pos = s1.players[playerId].pos;
                    auto offset = s1Pos - s0Pos;
                    auto s2Pos = PredPlayerPos(s1Pos, offset, EXTRAPOLATION_DURATION);
                    exS.players[playerId].pos = s2Pos;

                    // Interpolate
                    auto t1 = TickToTime(s1.serverTick);
                    auto t2 = TickToTime(s1.serverTick) + EXTRAPOLATION_DURATION;
                    auto ws = Math::Interpolation::GetWeights(t1.count(), t2.count(), renderTime.count());
                    auto alpha = ws.x;
                    EntityInterpolation(playerId, s1, exS, alpha);
                }
            }
        }
    }
}

void InGame::EntityInterpolation(Entity::ID playerId, const Networking::Snapshot& s1, const Networking::Snapshot& s2, float alpha)
{
    auto pos1 = s1.players.at(playerId).pos;
    auto pos2 = s2.players.at(playerId).pos;

#ifdef _DEBUG
    auto diff = glm::length(pos2 - pos1);
    client_->logger->LogDebug("P1 " + glm::to_string(pos1) + " P2 " + glm::to_string(pos2) + " D " + std::to_string(diff));
#endif

    auto smoothPos = pos1 * alpha + pos2 * (1 - alpha);
    playerTable[playerId].transform.position = smoothPos;
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
            break;
        }
        camController_.HandleSDLEvent(e);
    }
}

void InGame::Render()
{
    // Clear Buffer
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    DrawScene();
    DrawGUI();

    // Swap buffers
    SDL_GL_SwapWindow(client_->window_);

    // Max FPS Correction
    auto renderTime = Util::Time::GetTime() - preSimulationTime;
    if(renderTime < minFrameInterval)
    {
        Util::Time::Seconds diff = minFrameInterval - renderTime;
        //logger->LogInfo("Sleeping for " + std::to_string(diff.count()) + " s");
        //Util::Time::Sleep(diff);
    }
}

void InGame::DrawScene()
{
    // Draw data
    auto view = camera_.GetProjViewMat();
    
    for(auto player : playerTable)
    {
        // Start Debug
        auto playerId = player.first;
        auto oldPos = prevPlayerPos[playerId].transform.position;
        auto newPos = player.second.transform.position;
        auto diff = newPos - oldPos;
        auto distDiff = glm::length(diff);
        if(distDiff > 0.05f)
        {
            auto expectedDiff = PLAYER_SPEED *  deltaTime.count();
            client_->logger->LogDebug("Render: Player " + std::to_string(player.first) + " Prev " + glm::to_string(oldPos)
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
        // End debug

        auto t = player.second.transform.GetTransformMat();
        auto transform = view * t;
        shader.SetUniformMat4("transform", transform);
        shader.SetUniformVec4("color", glm::vec4{0.0f, 0.0f, 1.0f, 1.0f});
        shader.SetUniformInt("dmg", player.second.onDmg);
        //cube.Draw(shader, drawMode);
        playerModel.Draw(transform);
    }
    prevPlayerPos = playerTable;

    // Bind textures
    map_.tPalette.GetTextureArray()->Bind(GL_TEXTURE0);
    chunkShader.SetUniformInt("textureArray", 0);
    map_.cPalette.GetTextureArray()->Bind(GL_TEXTURE1);
    chunkShader.SetUniformInt("colorArray", 1);

    // Draw map
    map_.Draw(chunkShader, view);
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
    }
    
    // Draw GUI
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData(), guiVao.GetHandle());
    auto windowSize = client_->GetWindowSize();
    int scissor_box[4] = { 0, 0, windowSize.x, windowSize.y };
    ImGui_ImplOpenGL3_RestoreState(scissor_box);
}

