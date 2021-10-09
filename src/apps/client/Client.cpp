#include <Client.h>

#include <util/Random.h>

#include <iostream>
#include <algorithm>
#include <chrono>

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

    // Init players
    players = {
        {1, Math::Transform{glm::vec3{-7.0f, 4.15f, -7.0f}, glm::vec3{0.0f}, glm::vec3{0.5f, 1.0f, 0.5f}}},
        {2, Math::Transform{glm::vec3{-7.0f, 4.15f, 7.0f}, glm::vec3{0.0f}, glm::vec3{0.5f, 1.0f, 0.5f}}},
        {3, Math::Transform{glm::vec3{7.0f, 4.15f, -7.0f}, glm::vec3{0.0f}, glm::vec3{0.5f, 1.0f, 0.5f}}},
        {4, Math::Transform{glm::vec3{7.0f, 4.15f, 7.0f}, glm::vec3{0.0f}, glm::vec3{0.5f, 1.0f, 0.5f}}},
    };

    for(auto player : players)
    {
        auto playerId = player.first;
        GeneratePlayerTarget(playerId);
    }

    // Networking
    auto serverAddress = ENet::Address::CreateByIPAddress("127.0.0.1", 8080).value();
    host.SetOnConnectCallback([this](auto id)
    {
        this->logger->LogInfo("Succes on connection to server");
    });
    host.SetOnRecvCallback([this](auto id, auto channel, ENet::RecvPacket packet)
    {
        this->logger->LogInfo("Server packet recv of size: " + std::to_string(packet.GetSize()));
        uint32_t* tick = (uint32_t*) packet.GetData();
        logger->LogInfo("Server packet with data: " + std::to_string(*tick));
    });
    host.Connect(serverAddress);
}

void BlockBuster::Client::Update()
{
    double previous = GetCurrentTime();
    double lag = 0.0;

    logger->LogInfo("Update rate(s) is: " + std::to_string(UPDATE_RATE));
    while(!quit)
    {
        auto now = GetCurrentTime();
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

    // Networking
    for(auto player : players)
        PlayerUpdate(player.first, deltaTime);
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

void Client::UpdateNetworking()
{
    host.PollEvent(0);
}

void BlockBuster::Client::DrawScene()
{
    // Clear Buffer
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw data
    auto view = camera_.GetProjViewMat();

    for(auto players : players)
    {
        auto t = players.second.GetTransformMat();
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

void BlockBuster::Client::GeneratePlayerTarget(unsigned int playerId)
{
    auto x = Util::Random::Uniform(-14.f, 14.f);
    auto z = Util::Random::Uniform(-14.f, 14.f);
    playerTargets[playerId] = glm::vec3{x, 4.15f, z};
}

void BlockBuster::Client::PlayerUpdate(unsigned int playerId, double deltaTime)
{
    auto& player = players[playerId];
    auto playerTarget = playerTargets[playerId];

    auto moveDir = glm::normalize(playerTarget - player.position);
    auto velocity = moveDir * PLAYER_SPEED * (float)UPDATE_RATE ;
    player.position += velocity;

    auto dist = glm::length(playerTarget - player.position);
    if(dist < 1.0f)
        GeneratePlayerTarget(playerId);
}

double BlockBuster::Client::GetCurrentTime() const
{
    using namespace std::chrono;
    return duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count() / 1e9;
}