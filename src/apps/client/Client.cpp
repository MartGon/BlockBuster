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
    auto serverAddress = ENet::Address::CreateByIPAddress("127.0.0.1", 8082).value();
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
            this->startTime = Util::Time::GetUNIXTimeMS<uint64_t>();
            this->connected = true;
        }
        else if(packet->header.type == Networking::Command::Type::PLAYER_POS_UPDATE)
        {
            logger->LogInfo("Recv server snapshot for tick: " + std::to_string(packet->header.tick));
            Networking::Command::Server::PlayerUpdate playerUpdate = packet->data.playerUpdate;
            auto playerId = playerUpdate.playerId;

            // Create player entries
            if(snapshotHistory_.find(playerId) == snapshotHistory_.end())
            {
                snapshotHistory_[playerId] = Util::Queue<Networking::Command::Server::PlayerUpdate>{64};
            }
            if(playerTable.find(playerId) == playerTable.end())
            {
                Math::Transform transform{playerUpdate.pos, glm::vec3{0.0f}, glm::vec3{0.5f, 1.0f, 0.5f}};
                playerTable[playerId] = Entity::Player{playerId, transform};
            }
            snapshotHistory_[playerId].Push(playerUpdate);
        }
        else if(packet->header.type == Networking::Command::Type::PLAYER_DISCONNECTED)
        {
            Networking::Command::Server::PlayerDisconnected playerDisconnect = packet->data.playerDisconnect;
            logger->LogInfo("Player with id " + std::to_string(playerDisconnect.playerId) + " disconnected");

            playerTable.erase(playerDisconnect.playerId);
            snapshotHistory_.erase(playerDisconnect.playerId);
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
    // Update players' pos
    for(auto pair : snapshotHistory_)
    {
        auto playerId = pair.first;
        auto playerUpdate = pair.second.Back();
        if(playerUpdate.has_value() && playerId != this->playerId)
        {
            auto& player = this->playerTable[playerId];
            player.transform.position = playerUpdate->pos;
        }
    }

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
    auto recvPos = snapshotHistory_[playerId].Back();
    if(recvPos && prediction)
    {
        auto newPos = recvPos->pos;
        auto pPos = prediction->pos;
        auto diff = glm::length(newPos - pPos);
        // On error
        if(diff > 0.05)
        {
            // Accept player pos
            logger->LogInfo("There was a " + std::to_string(diff) + " difference for cmd " 
                + std::to_string(this->lastAck));
            logger->LogInfo("Pred pos " + glm::to_string(pPos));
            logger->LogInfo("Recv pos " + glm::to_string(newPos));
            player.transform.position = newPos;

            // Run prev commands again
            logger->LogInfo("Predicting commands ahead of cmd " + std::to_string(lastAck));
            for(auto i = 0; i < predictionHistory_.GetSize(); i++)
            {
                auto predict = predictionHistory_.At(i);
                auto move = predict->cmd.data.playerMovement;
                MovePlayer(move);
            }
        }
    }

    // Run predicted command for this simulation
    auto predictedPos = MovePlayer(cmd.data.playerMovement);
    logger->LogInfo("Predicted pos for cmd " + std::to_string(cmdId) + ": " + glm::to_string(predictedPos));
    Prediction p{cmdId, cmd, predictedPos};
    predictionHistory_.Push(p);
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
