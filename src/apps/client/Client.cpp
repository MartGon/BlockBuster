#include <Client.h>

#include <util/Random.h>

#include <iostream>
#include <algorithm>
#include <chrono>

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
}

void BlockBuster::Client::Update()
{
    using namespace std::chrono;
    auto prev = duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
    DoUpdate(deltaTime);
    auto now = duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
    deltaTime = (now - prev) / 1000000.f;
    logger->LogInfo("Elapsed time: " + std::to_string(deltaTime));
}

void BlockBuster::Client::DoUpdate(float deltaTime)
{
    HandleSDLEvents();
    camController_.Update();

    // Clear Buffer
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    DrawScene();

    SDL_GL_SwapWindow(window_);
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
}