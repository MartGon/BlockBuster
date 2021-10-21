#include <Client.h>

#include <util/Random.h>
#include <util/Time.h>

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
    auto serverAddress = ENet::Address::CreateByIPAddress("127.0.0.1", 8081).value();
    host.SetOnConnectCallback([this](auto id)
    {
        this->serverId = id;
        this->logger->LogInfo("Succes on connection to server");
    });
    host.SetOnRecvCallback([this](auto id, auto channel, ENet::RecvPacket ePacket)
    {
        auto* packet = (Networking::Command*) ePacket.GetData();
        this->serverTick = packet->header.tick;

        //this->logger->LogInfo(std::to_string(this->GetCurrentTime()) + " Tick: " + std::to_string(serverTick) + " Server packet recv of size: " + std::to_string(ePacket.GetSize()) );
        if(packet->header.type == Networking::Command::Type::CLIENT_CONFIG)
        {
            auto config = packet->data.config;
            logger->LogInfo("Server tick rate is " + std::to_string(config.sampleRate));
            this->serverTickRate = config.sampleRate;
            this->playerId = config.playerId;
            this->connected = true;

            this->startTime = Util::Time::GetUNIXTimeMS<uint64_t>();
            this->startServerTick = this->serverTick;
        }
        else if(packet->header.type == Networking::Command::Type::PLAYER_POS_UPDATE)
        {
            Networking::Command::Server::PlayerUpdate playerUpdate = packet->data.playerUpdate;
            auto playerId = playerUpdate.playerId;
            logger->LogInfo("Recv server snapshot for tick: " + std::to_string(packet->header.tick) + " p " + std::to_string(playerId));

            // Create player entries
            if(playerTable.find(playerId) == playerTable.end())
            {
                Math::Transform transform{playerUpdate.pos, glm::vec3{0.0f}, glm::vec3{0.5f, 1.0f, 0.5f}};
                playerTable[playerId] = Entity::Player{playerId, transform};
            }

            // Store snapshot. FIXME/TODO: Needs to batch player updates on server first.
            auto now = GetCurrentTime();
            auto snapshot = snapshotHistory.FindFirstPair([this](auto i, auto s){
                return s.serverTick == this->serverTick;
            });
            if(snapshot)
            {
                snapshot->second.playerPositions[playerId] = playerUpdate;
                snapshotHistory.Set(snapshot->first, snapshot->second);
            }
            else
            {
                Snapshot snapshot{now, packet->header.tick, {{playerId, playerUpdate}}};
                snapshotHistory.Push(snapshot);
            }
        }
        else if(packet->header.type == Networking::Command::Type::PLAYER_DISCONNECTED)
        {
            Networking::Command::Server::PlayerDisconnected playerDisconnect = packet->data.playerDisconnect;
            logger->LogInfo("Player with id " + std::to_string(playerDisconnect.playerId) + " disconnected");

            playerTable.erase(playerDisconnect.playerId);
        }
        else if(packet->header.type == Networking::Command::Type::ACK_COMMAND)
        {
            auto ack = packet->data.ackCommand;
            this->lastAck = ack.commandId;
            logger->LogInfo("Recv server ACK: " + std::to_string(this->lastAck));
        }
    });
    host.Connect(serverAddress);

    auto attempts = 0;
    while(!connected && attempts < 5)
    {
        Util::Time::SleepMS(500);
        logger->LogInfo("Connecting to server...");
        host.PollAllEvents();
        attempts++;
    }

    if(!connected)
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
        auto now =  Util::Time::GetUNIXTime();
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
    //EntityInterpolation();

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

uint64_t Client::GetCurrentTime()
{
    return Util::Time::GetUNIXTimeMS<uint64_t>() - startTime;
}

void Client::PredictPlayerMovement(Networking::Command cmd, uint32_t cmdId)
{
    auto& player = playerTable[playerId];

    // Discard old commands
    while(predictionHistory_.GetSize() > 0 && predictionHistory_.Front()->cmdId <= lastAck)
    {
        auto cmd = predictionHistory_.Pop();
    }
    
    // Checking prediction errors
    auto prediction = predictionHistory_.FindFirst([this](auto predict){
        return predict.cmdId == this->lastAck;
    });
    auto lastSnapshot = snapshotHistory.Back();
    if(prediction && lastSnapshot.has_value())
    {
        if(lastSnapshot->playerPositions.find(playerId) != lastSnapshot->playerPositions.end())
        {
            auto newPos = lastSnapshot->playerPositions[playerId].pos;
            auto pPos = prediction->pos;
            auto diff = glm::length(newPos - pPos);
            // On error
            if(diff > 0.05)
            {
                // Accept player pos
                //logger->LogInfo("There was a " + std::to_string(diff) + " difference for cmd " 
                    //+ std::to_string(this->lastAck));
                //logger->LogInfo("Pred pos " + glm::to_string(pPos));
                //logger->LogInfo("Recv pos " + glm::to_string(newPos));
                player.transform.position = newPos;

                // Run prev commands again
                //logger->LogInfo("Predicting commands ahead of cmd " + std::to_string(lastAck));
                for(auto i = 0; i < predictionHistory_.GetSize(); i++)
                {
                    auto predict = predictionHistory_.At(i);
                    auto move = predict->cmd.data.playerMovement;
                    MovePlayer(move);
                }
            }
        }
    }

    // Run predicted command for this simulation
    auto predictedPos = MovePlayer(cmd.data.playerMovement);
    //logger->LogInfo("Predicted pos for cmd " + std::to_string(cmdId) + ": " + glm::to_string(predictedPos));
    Prediction p{cmdId, cmd, predictedPos};
    predictionHistory_.Push(p);
}

void Client::EntityInterpolation()
{
    auto clientTime = GetCurrentTime();
    auto serverTickRateMs = this->serverTickRate *1e3;
    auto windowSize = 2.0 * serverTickRateMs;
    uint64_t renderTime = clientTime - windowSize;

    if(renderTime < 0)
        return;

    // Get last snapshot before renderTime
    auto s1 = snapshotHistory.Back();
    auto s2i = 0;
    for(auto i = 0; i < snapshotHistory.GetSize(); i++)
    {
        auto s = snapshotHistory.At(i).value();
        if(s.arrivalTime <= renderTime)
        {
            s1 = s;
            s2i = i + 1;
        }
        else
            break;
    }
    auto s2 = snapshotHistory.At(s2i);

    if(s1.has_value() && s2.has_value())
    {        
        // Find weights
        auto d = s2->arrivalTime - s1->arrivalTime;
        auto d1 = renderTime - s1->arrivalTime;
        // auto d2 = s2->arrivalTime - renderTime;
        auto w1 = (1.0f - (float)d1 / (float)d);
        //auto w2 = (float)d2 / (float)d;
        auto w2 = 1.0f - w1;

        for(auto pair : playerTable)
        {
            auto playerId = pair.first;
            if(playerId == this->playerId)
                continue;
            logger->LogInfo("Interpolation s1 tick " + std::to_string(s1->serverTick) + " s2 tick " + std::to_string(s2->serverTick));

            auto pos1 = pair.second.transform.position;
            auto pos2 = pair.second.transform.position;

            auto hasS1 = s1->playerPositions.find(playerId) != s1->playerPositions.end();
            auto hasS2 = s2->playerPositions.find(playerId) != s2->playerPositions.end();
            if(hasS1)
                pos1 = s1->playerPositions[playerId].pos;
            else
                logger->LogInfo("Could not find data S1 for player " + std::to_string(playerId));
            if(hasS2)
                pos2 = s2->playerPositions[playerId].pos;
            else
                logger->LogInfo("Could not find data S2 for player " + std::to_string(playerId));

            if(hasS1 && hasS2)
            {
                auto smoothPos = pos1 * w1 + pos2 * w2;
                auto oldPos = playerTable[playerId].transform.position;
                auto diff = smoothPos.x - oldPos.x;
                auto dist = glm::length(diff);
                logger->LogInfo("Player moved by dist " + std::to_string(dist));
                if(diff < -0.005)
                {
                    logger->LogInfo("ERROR!!! Player didn't move right");
                    //quit = true;
                }
                //else
                    playerTable[playerId].transform.position = smoothPos;

                logger->LogInfo("Rendering time " + std::to_string(renderTime));
                logger->LogInfo("S1 tick " + std::to_string(s1->serverTick) + " S2 tick " + std::to_string(s2->serverTick));
                logger->LogInfo("S1 time " + std::to_string(s1->arrivalTime) + " S2 time " + std::to_string(s2->arrivalTime));
                logger->LogInfo("Moved from " + glm::to_string(oldPos) + " to " + glm::to_string(smoothPos));
                logger->LogInfo("S1 pos " + glm::to_string(pos1) + " S2 pos " + glm::to_string(pos2));
                logger->LogInfo("W1 " + std::to_string(w1) + " W2 " + std::to_string(w2));
                logger->LogInfo("D " + std::to_string(d));
            }
        }
    }
    else
    {
        logger->LogError("There were not enough snapshots to interpolate");
    }
}

glm::vec3 Client::MovePlayer(Networking::Command::User::PlayerMovement move)
{
    auto& player = playerTable[playerId];
    const float PLAYER_SPEED = 5.f;
    auto velocity = move.moveDir * PLAYER_SPEED * (float)serverTickRate;
    player.transform.position += velocity;
    return player.transform.position;
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
        auto diff = (minFrameInterval - renderTime) * 1e3;
        Util::Time::SleepMS(diff);
    }
}

void BlockBuster::Client::DrawScene()
{
    // Draw data
    auto view = camera_.GetProjViewMat();

    for(auto player : playerTable)
    {
        auto t = player.second.transform.GetTransformMat();
        auto transform = view * t;
        shader.SetUniformMat4("transform", transform);
        shader.SetUniformVec4("color", glm::vec4{1.0f});
        cylinder.Draw(shader, drawMode);
    }

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

            ImGui::End();
        }

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

            ImGui::End();
        }
    }
    
    // Draw GUI
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
