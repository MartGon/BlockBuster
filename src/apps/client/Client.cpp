#include <Client.h>

#include <iostream>
#include <algorithm>

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
    camera_.SetPos(glm::vec3{0, 2, 3});
    camera_.SetTarget(glm::vec3{0});
    auto winSize = GetWindowSize();
    camera_.SetParam(Rendering::Camera::Param::ASPECT_RATIO, (float)winSize.x / (float)winSize.y);
    camera_.SetParam(Rendering::Camera::Param::FOV, config.window.fov);
    camController_ = ::App::Client::CameraController{&camera_, {window_, io_}, ::App::Client::CameraMode::EDITOR};

    // Map
    Game::Display display{Game::DisplayType::COLOR, 0};
    Game::BlockRot rot{Game::ROT_0, Game::ROT_0};
    for(int x = -8; x < 8; x++)
    {
        for(int z = -8; z < 8; z++)
        {
            map_.SetBlock(glm::ivec3{x, 0, z}, Game::Block{Game::BlockType::BLOCK, rot, display});
        }
    }
}

void BlockBuster::Client::Update()
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
    glm::mat4 t{1.f};
    auto view = camera_.GetProjViewMat();
    auto transform = view * t;
    shader.SetUniformMat4("transform", transform);
    shader.SetUniformVec4("color", glm::vec4{1.0f});
    cylinder.Draw(shader, drawMode);

    // Draw map
    map_.Draw(chunkShader, view);
}