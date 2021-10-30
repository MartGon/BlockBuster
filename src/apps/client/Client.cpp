#include <Client.h>

#include <util/Random.h>
#include <util/Time.h>

#include <math/Interpolation.h>

#include <debug/Debug.h>

#include <iostream>
#include <algorithm>

using namespace BlockBuster;

Client::Client(::App::Configuration config) : App{config}, 
    host{ENet::HostFactory::Get()->CreateHost(1, 2)}
{
}

void BlockBuster::Client::Start()
{
    // GL features
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // Shaders
    // Load shaders
    try{
        shader = GL::Shader::FromFolder(config.openGL.shadersFolder, "circleVertex.glsl", "circleFrag.glsl");
        chunkShader = GL::Shader::FromFolder(config.openGL.shadersFolder, "chunkVertex.glsl", "chunkFrag.glsl");
    }
    catch(const std::runtime_error& e)
    {
        this->logger->LogCritical(e.what());
        quit = true;
        return;
    }

    // Meshes
    cylinder = Rendering::Primitive::GenerateCylinder(2.f, 4.f, 32, 1);

    // Camera
    camera_.SetPos(glm::vec3{0, 8, 16});
    camera_.SetTarget(glm::vec3{0});
    auto winSize = GetWindowSize();
    camera_.SetParam(Rendering::Camera::Param::ASPECT_RATIO, (float)winSize.x / (float)winSize.y);
    camera_.SetParam(Rendering::Camera::Param::FOV, config.window.fov);
    camController_ = ::App::Client::CameraController{&camera_, {window_, io_}, ::App::Client::CameraMode::EDITOR};

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
    auto serverAddress = ENet::Address::CreateByIPAddress("127.0.0.1", 8082).value();
    host.SetOnConnectCallback([this](auto id)
    {
        this->serverId = id;
        this->logger->LogInfo("Succes on connection to server");
    });
    host.SetOnRecvCallback([this](auto id, auto channel, ENet::RecvPacket ePacket)
    {
        auto* packet = (Networking::Command*) ePacket.GetData();

        if(packet->header.type == Networking::Command::Type::CLIENT_CONFIG)
        {
            auto config = packet->data.config;
            logger->LogInfo("Server tick rate is " + std::to_string(config.sampleRate));
            this->serverTickRate = config.sampleRate;
            this->playerId = config.playerId;
            this->connected = true;
        }
        else if(packet->header.type == Networking::Command::Type::SERVER_SNAPSHOT)
        {
            logger->LogInfo("Recv server snapshot for tick: " + std::to_string(packet->header.tick));

            // Entity Interpolation - Reset offset
            auto mostRecent = this->GetMostRecentSnapshot();
            if(mostRecent.has_value() && packet->header.tick > mostRecent->serverTick)
                this->offsetMillis = 0;

            // Process Snapshot
            auto update = packet->data.snapshot;

            // Update last ack
            this->lastAck = update.lastCmd;
            logger->LogInfo("Server ack  command: " + std::to_string(update.lastCmd));

            // Update player locations
            Util::Buffer::Reader reader{packet, ePacket.GetSize()};
            reader.Skip(sizeof(Networking::Command));
            auto s = Networking::Snapshot::FromBuffer(reader);
            for(auto player : s.players)
            {
                // Create player entries
                auto playerId = player.first;
                if(playerTable.find(playerId) == playerTable.end())
                {
                    Math::Transform transform{player.second.pos, glm::vec3{0.0f}, glm::vec3{0.5f, 1.0f, 0.5f}};
                    playerTable[playerId] = Entity::Player{playerId, transform};
                }
            }

            // Store snapshot
            snapshotHistory.Push(s);
        }
        else if(packet->header.type == Networking::Command::Type::PLAYER_DISCONNECTED)
        {
            Networking::Command::Server::PlayerDisconnected playerDisconnect = packet->data.playerDisconnect;
            logger->LogInfo("Player with id " + std::to_string(playerDisconnect.playerId) + " disconnected");

            playerTable.erase(playerDisconnect.playerId);
        }
    });
    host.Connect(serverAddress);

    auto attempts = 0;
    while(!connected && attempts < 5)
    {
        Util::Time::SleepMillis(500);
        logger->LogInfo("Connecting to server...");
        host.PollAllEvents();
        attempts++;
    }

    if(connected)
    {
        uint32_t maxTick = 0;
        for(auto i = 0;i < snapshotHistory.GetSize();i++)
        {
            auto s = snapshotHistory.At(i);
            maxTick = std::max(s->serverTick, maxTick);
        }
    }
    else
    {
        logger->LogInfo("Could not connect to server. Quitting");
    }

    quit = !connected;
}

void BlockBuster::Client::Update()
{
    prevRenderTime = Util::Time::GetUNIXTime();
    double lag = serverTickRate;

    logger->LogInfo("Update rate(s) is: " + std::to_string(serverTickRate));
    while(!quit)
    {
        auto now = Util::Time::GetUNIXTime();
        frameInterval = (now - prevRenderTime);

        prevRenderTime = now;
        lag += frameInterval;

        HandleSDLEvents();
        if(connected)
            RecvServerSnapshots();

        while(lag >= serverTickRate)
        {
            DoUpdate(serverTickRate);

            if(connected)
                UpdateNetworking();

            lag -= serverTickRate;
        }

        EntityInterpolation();
        SmoothPlayerMovement();
        Render();
    }
}

bool BlockBuster::Client::Quit()
{
    return quit;
}

void BlockBuster::Client::DoUpdate(double deltaTime)
{
    camController_.Update();
}

void Client::RecvServerSnapshots()
{
    host.PollAllEvents();
}

void Client::UpdateNetworking()
{
    // Sample player input
    int8_t* state = (int8_t*)SDL_GetKeyboardState(nullptr);
    glm::vec3 moveDir{0.0f};
    moveDir.x = state[SDL_SCANCODE_KP_6] - state[SDL_SCANCODE_KP_4];
    moveDir.z = state[SDL_SCANCODE_KP_2] - state[SDL_SCANCODE_KP_8];
    auto len = glm::length(moveDir);
    moveDir = len > 0 ? moveDir / len : moveDir;

    // Send player inputs
    auto cmdId = this->cmdId++;
    Networking::Command::Header header;
    header.type = Networking::Command::Type::PLAYER_MOVEMENT;
    header.tick = cmdId;

    Networking::Command::User::PlayerMovement playerMovement;
    playerMovement.moveDir = moveDir;

    Networking::Command::Payload data;
    data.playerMovement = playerMovement;
    
    Networking::Command cmd{header, data};
    ENet::SentPacket sentPacket{&cmd, sizeof(cmd), ENetPacketFlag::ENET_PACKET_FLAG_RELIABLE};
    host.SendPacket(serverId, 0, sentPacket);

    // Prediction
    PredictPlayerMovement(cmd, cmdId);
}

void Client::PredictPlayerMovement(Networking::Command cmd, uint32_t cmdId)
{
    auto& player = playerTable[playerId];

    // Discard old commands
    std::optional<Prediction> prediction;
    while(predictionHistory_.GetSize() > 0 && predictionHistory_.Front()->cmdId <= lastAck)
    {
        prediction = predictionHistory_.Pop();
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
                logger->LogError("Prediction: Error on prediction");
                logger->LogError("Prediction: Tick " + std::to_string(lastSnapshot->serverTick) + " ACK " + std::to_string(this->lastAck));
                logger->LogError("Prediction: D " + std::to_string(diff) + " P " + glm::to_string(pPos) + " S " + glm::to_string(newPos));

                // Accept player pos
                auto playerPos = newPos;

                // Run prev commands again
                for(auto i = 0; i < predictionHistory_.GetSize(); i++)
                {
                    auto predict = predictionHistory_.At(i);

                    // Re-calculate prediction
                    auto move = predict->cmd.data.playerMovement;
                    auto origin = playerPos;
                    playerPos = PredPlayerPos(playerPos, move.moveDir, serverTickRate);

                    // Update history
                    predict->origin = origin;
                    predict->dest = playerPos;
                    predictionHistory_.Set(i, predict.value());
                }

                // Update error correction values
                errorCorrectionDiff = pPos - newPos;
                errorCorrectionStart = Util::Time::GetUNIXTime();
            }
        }
    }
    // Get prev pos
    auto prevPos = playerTable[playerId].transform.position;
    if(auto prevPred = predictionHistory_.Back())
        prevPos = prevPred->dest;

    // Run predicted command for this simulation
    auto now = Util::Time::GetUNIXTime();
    auto predictedPos = PredPlayerPos(prevPos, cmd.data.playerMovement.moveDir, serverTickRate);
    Prediction p{cmdId, cmd, prevPos, predictedPos, now};
    predictionHistory_.Push(p);
}

void Client::SmoothPlayerMovement()
{
    auto last = predictionHistory_.Back();
    auto& renderPos = playerTable[playerId].transform.position;
    if(last)
    {
        auto now = Util::Time::GetUNIXTime();
        auto elapsed = now - last->time;
        
        auto origin = last->origin;
        auto predPos = PredPlayerPos(origin, last->cmd.data.playerMovement.moveDir, elapsed);

        // Error correction
        double errorElapsed = now - errorCorrectionStart;
        float weight = glm::max(1.0 - (errorElapsed / ERROR_CORRECTION_DURATION), 0.0);
        glm::vec3 errorCorrection = errorCorrectionDiff * weight;
        auto dist = glm::length(errorCorrection);
        if(dist > 0)
            logger->LogError("Error correction is " + glm::to_string(errorCorrection) + " W " + std::to_string(weight) + " D " + std::to_string(dist));
        renderPos = predPos + errorCorrection;
    }
}

glm::vec3 Client::PredPlayerPos(glm::vec3 pos, glm::vec3 moveDir, float deltaTime)
{
    auto& player = playerTable[playerId];
    auto velocity = moveDir * PLAYER_SPEED * (float)deltaTime;
    auto predPos = pos + velocity;
    return predPos;
}

std::optional<Networking::Snapshot> Client::GetMostRecentSnapshot()
{
    uint32_t maxTick = 0;
    std::optional<Networking::Snapshot> mostRecent;
    for(auto i = 0; i < snapshotHistory.GetSize(); i++)
    {
        auto s = snapshotHistory.At(i);
        if(s->serverTick > maxTick)
        {
            maxTick = s->serverTick;
            mostRecent = s;
        }
    }

    return mostRecent;
}

uint64_t Client::TickToMillis(uint32_t tick)
{
    return (double)tick * this->serverTickRate * 1e3;
}

uint64_t Client::GetCurrentTime()
{
    auto maxTick = GetMostRecentSnapshot()->serverTick;
    uint64_t base = TickToMillis(maxTick);

    return base + this->offsetMillis;
}

uint64_t Client::GetRenderTime()
{
    auto clientTime = GetCurrentTime();
    double serverTickRateMillis = this->serverTickRate * 1e3;
    uint64_t windowSize = 2.0 * serverTickRateMillis;
    uint64_t renderTime = clientTime - windowSize;
    
    return renderTime;
}

void Client::EntityInterpolation()
{
    if(snapshotHistory.GetSize() < 2)
        return;

    // Calculate render time
    auto clientTime = GetCurrentTime();
    uint64_t renderTime = GetRenderTime();

    if(renderTime < 0)
        return;

    // Get first snapshot after renderTime
    auto s2p = snapshotHistory.FindFirstPair([this, renderTime](auto i, auto s)
    {
        uint64_t time = TickToMillis(s.serverTick);
        return time > renderTime;
    });

    if(s2p.has_value())
    {
        // Find samples to use for interpolation
        // Sample 1: Last one before current render time
        // Sample 2: First one after current render time
        auto s1 = snapshotHistory.At((int)s2p->first - 1);
        auto s2 = s2p->second;

        if(s1.has_value())
        {   
            // Find weights
            auto t1 = TickToMillis(s1->serverTick);
            auto t2 = TickToMillis(s2.serverTick);
            auto ws = Math::Interpolation::GetWeights(t1, t2, renderTime);
            auto w1 = ws.x; auto w2 = ws.y;

            logger->LogDebug("RT " + std::to_string(renderTime) + " CT " + std::to_string(clientTime) + " OM " + std::to_string(offsetMillis));
            logger->LogDebug("T1 " + std::to_string(t1) + " T2 " + std::to_string(t2));
            logger->LogDebug("W1 " + std::to_string(w1) + " W2 " + std::to_string(w2));
            logger->LogDebug("Tick 1 " + std::to_string(s1->serverTick) + " Tick 2 " + std::to_string(s2.serverTick));

            EntityInterpolation(s1.value(), s2, w1);
        }
    }
    else
    {
        auto lastSample = GetMostRecentSnapshot();
        if(lastSample->serverTick != extrapolatedSnapshot.serverTick)
        {
            logger->LogError("There were not enough snapshots to interpolate. Performing extrapolation instead");
            CalculateExtrapolatedSnapshot();
        }

        EntityExtrapolation();
    }

    offsetMillis += (uint64_t)(frameInterval * 1e3);
}

void Client::EntityInterpolation(Networking::Snapshot s1, Networking::Snapshot s2, float alpha)
{
    for(auto pair : playerTable)
    {
        auto playerId = pair.first;
        // No need to do interpolation for player char
        if(playerId == this->playerId)
            continue;

        auto hasS1 = s1.players.find(playerId) != s1.players.end();
        auto hasS2 = s2.players.find(playerId) != s2.players.end();
        if(hasS1 && hasS2)
        {
            auto pos1 = s1.players[playerId].pos;
            auto pos2 = s2.players[playerId].pos;
            auto smoothPos = pos1 * alpha + pos2 * (1 - alpha);
            playerTable[playerId].transform.position = smoothPos;
        }
    }
}

void Client::CalculateExtrapolatedSnapshot()
{
    auto lastIndex = snapshotHistory.GetSize() - 1;
    auto lastSample = snapshotHistory.At(lastIndex).value();
    auto prevSample = snapshotHistory.At(lastIndex - 1).value();

    logger->LogError("Last tick recv is from " + std::to_string(lastSample.serverTick));

    this->extrapolatedSnapshot = lastSample;
    for(auto pair : playerTable)
    {
        auto lastPos = lastSample.players[pair.first].pos;
        auto prevPos = prevSample.players[pair.first].pos;
        auto moveDir = lastPos - prevPos;
        auto extrapolatedPos = PredPlayerPos(lastPos, moveDir, EXTRAPOLATION_DURATION);

        this->extrapolatedSnapshot.players[pair.first].pos = extrapolatedPos;
    }
}

void Client::EntityExtrapolation()
{
    auto renderTime = GetRenderTime();
    auto s1 = snapshotHistory.At(-1).value();
    auto s2 = extrapolatedSnapshot;

    // Find weights
    auto t1 = TickToMillis(s1.serverTick);
    uint64_t t2 = TickToMillis(s2.serverTick) + EXTRAPOLATION_DURATION * 1e3;
    auto ws = Math::Interpolation::GetWeights(t1, t2, renderTime);
    auto w1 = ws.x; auto w2 = ws.y;

    logger->LogError("ExtraPolation T1 " + std::to_string(t1) + " T2 " + std::to_string(t2));
    logger->LogError("ExtraPolation W1 " + std::to_string(w1) + " W2 " + std::to_string(w2));

    // Interpolate 
    EntityInterpolation(s1, s2, w1);
}

void BlockBuster::Client::HandleSDLEvents()
{
    SDL_Event e;
    
    while(SDL_PollEvent(&e) != 0)
    {
        ImGui_ImplSDL2_ProcessEvent(&e);

        switch(e.type)
        {
        case SDL_QUIT:
            quit = true;
            break;
        case SDL_KEYDOWN:
            if(e.key.keysym.sym == SDLK_f)
                drawMode = drawMode == GL_FILL ? GL_LINE : GL_FILL;
            break;
        }
        camController_.HandleSDLEvent(e);
    }
}

void Client::Render()
{
    // Clear Buffer
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    DrawScene();
    DrawGUI();

    // Swap buffers
    SDL_GL_SwapWindow(window_);

    // Max FPS Correction
    auto renderTime = Util::Time::GetUNIXTime() - prevRenderTime;
    if(renderTime < minFrameInterval)
    {
        uint64_t diff = (minFrameInterval - renderTime) * 1e3;
        Util::Time::SleepMillis(diff);
    }
}

void BlockBuster::Client::DrawScene()
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
            // TODO: This issue is caused because the server sometimes handles 2 cmds in a tick. Causing a movement which is doubled in length for other players
            // This could be solved by batching updates on the server. Interpolation window should be changed accordingly
            
            auto expectedDiff = PLAYER_SPEED *  frameInterval;
            logger->LogDebug("Render: Player " + std::to_string(player.first) + " Prev " + glm::to_string(oldPos)
            + " New " + glm::to_string(newPos) + " Diff " + glm::to_string(diff) + " Frame interval " + std::to_string(frameInterval));

            auto offset = glm::abs(distDiff - expectedDiff);
            if(offset > 0.05f)
            {
                logger->LogDebug("Render: Difference is bigger than expected: ");
                logger->LogDebug("Found diff " + std::to_string(distDiff) + " Expected diff " + std::to_string(expectedDiff) + " Offset " + std::to_string(offset));
            }
        }
        // End debug

        auto t = player.second.transform.GetTransformMat();
        auto transform = view * t;
        shader.SetUniformMat4("transform", transform);
        shader.SetUniformVec4("color", glm::vec4{1.0f});
        cylinder.Draw(shader, drawMode);
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

void Client::DrawGUI()
{
    // Clear GUI buffer
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(window_);
    ImGui::NewFrame();

    bool isOpen = true;
    if(connected)
    {
        if(ImGui::Begin("Network Stats"))
        {
            auto info = host.GetPeerInfo(serverId);

            ImGui::Text("Config");
            ImGui::Separator();
            ImGui::Text("Server tick rate: %.2f ms", serverTickRate * 1e3);

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
            uint64_t frameIntervalMs = frameInterval * 1e3;
            ImGui::Text("Frame interval (delta time):");ImGui::SameLine();ImGui::Text("%lu ms", frameIntervalMs);
            uint64_t fps = 1.0 / frameInterval;
            ImGui::Text("FPS: %lu", fps);

            ImGui::InputDouble("Max FPS", &maxFPS, 1.0, 5.0, "%.0f");
            minFrameInterval = 1 / maxFPS;
            ImGui::Text("Min Frame interval:");ImGui::SameLine();ImGui::Text("%.0f ms", minFrameInterval * 1e3);
        }
        ImGui::End();
    }
    
    // Draw GUI
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
