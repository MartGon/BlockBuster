#include <Client.h>

#include <util/Random.h>
#include <util/Time.h>

#include <networking/Command.h>
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
    auto serverAddress = ENet::Address::CreateByIPAddress("127.0.0.1", 8080).value();
    host.SetOnConnectCallback([this](auto id)
    {
        this->serverId = id;
        this->logger->LogInfo("Succes on connection to server");
    });
    host.SetOnRecvCallback([this](auto id, auto channel, ENet::RecvPacket ePacket)
    {
        this->logger->LogInfo("Server packet recv of size: " + std::to_string(ePacket.GetSize()));
        
        auto* packet = (Networking::Packet*) ePacket.GetData();
        if(packet->header.type == Networking::Command::Type::PLAYER_UPDATE)
        {
            Networking::Command::Server::PlayerUpdate playerUpdate = packet->data.playerUpdate;
            logger->LogInfo("Server packet with player " + std::to_string(playerUpdate.playerId) + " data");
            logger->LogInfo("Player new pos is " + glm::to_string(playerUpdate.pos));
            if(playerTable.find(playerUpdate.playerId) == playerTable.end())
            {
                Math::Transform transform{playerUpdate.pos, glm::vec3{0.0f}, glm::vec3{1.0f}};
                playerTable[playerUpdate.playerId] = Entity::Player{playerUpdate.playerId, transform};
            }
            else
                this->playerTable[playerUpdate.playerId].transform.position = playerUpdate.pos;
        }
    });
    host.Connect(serverAddress);
}

void BlockBuster::Client::Update()
{
    double previous = Util::Time::GetCurrentTime();
    double lag = 0.0;

    logger->LogInfo("Update rate(s) is: " + std::to_string(UPDATE_RATE));
    while(!quit)
    {
        auto now =  Util::Time::GetCurrentTime();
        auto elapsed = (now - previous);
        
        previous = now;
        lag += elapsed;

        HandleSDLEvents();
        UpdateNetworking();

        while(lag >= UPDATE_RATE)
        {
            DoUpdate(UPDATE_RATE);
            lag -= UPDATE_RATE;
        }

        DrawScene();
    }
}

void BlockBuster::Client::DoUpdate(double deltaTime)
{
    camController_.Update();

    SendPlayerMovement();

    // Networking
}

bool BlockBuster::Client::Quit()
{
    return quit;
}

void BlockBuster::Client::HandleSDLEvents()
{
    SDL_Event e;
    
    while(SDL_PollEvent(&e) != 0)
    {
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

void BlockBuster::Client::DrawScene()
{
    // Clear Buffer
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

    // Swap buffers
    SDL_GL_SwapWindow(window_);
}

void Client::UpdateNetworking()
{
    host.PollEvent(0);

    // TODO: Create something like PollAllEvents which empties the event queue;
}

void Client::SendPlayerMovement()
{
    int8_t* state = (int8_t*)SDL_GetKeyboardState(nullptr);
    glm::vec3 moveDir{0.0f};
    moveDir.x = state[SDL_SCANCODE_KP_6] - state[SDL_SCANCODE_KP_4];
    moveDir.z = state[SDL_SCANCODE_KP_2] - state[SDL_SCANCODE_KP_8];
    logger->LogInfo(std::to_string(tickCount) + ": Move dir " + glm::to_string(moveDir));
    auto len = glm::length(moveDir);
    moveDir = len > 0 ? moveDir / len : moveDir;

    Networking::Packet::Header header;
    header.type = Networking::Command::Type::PLAYER_MOVEMENT;
    header.tick = tickCount;

    Networking::Command::User::PlayerMovement playerMovement;
    playerMovement.playerId = playerId;
    playerMovement.moveDir = moveDir;

    Networking::Packet::Payload data;
    data.playerMovement = playerMovement;
    
    Networking::Packet packet{header, data};

    ENet::SentPacket sentPacket{&packet, sizeof(packet), ENetPacketFlag::ENET_PACKET_FLAG_RELIABLE};
    host.SendPacket(serverId, 0, sentPacket);
}
