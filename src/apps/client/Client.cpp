#include <Client.h>

#include <iostream>
#include <algorithm>

void BlockBuster::Client::Start()
{
    // Shaders
    shader.Use();

    // Meshes
    cube = Rendering::Primitive::GenerateCube();
    slope = Rendering::Primitive::GenerateSlope();

    // Textures
    texture = GL::Texture::FromFolder(TEXTURES_DIR, "SmoothStone.png");
    gTexture = GL::Texture::FromFolder(TEXTURES_DIR, "SmoothStone.png");
    try
    {
        texture.Load();
        gTexture.Load();
    }
    catch(const GL::Texture::LoadError& e)
    {
        std::cout << "Error when loading texture " + e.path_.string() + ": " +  e.what() << '\n';
    }
    
    // OpenGL features
    glEnable(GL_DEPTH_TEST);

    // Player
    player.transform.scale = playerScale;

    // World
    blocks = {
        {Math::Transform{glm::vec3{1.0f, -1.0f, 0.0f} * scale, glm::vec3{0.0f, 0.0f, 0.0f}, scale}, Game::BLOCK}, 
        {Math::Transform{glm::vec3{1.0f, -1.0f, 1.0f} * scale, glm::vec3{0.0f, 0.0f, 0.0f}, scale}, Game::BLOCK},
        {Math::Transform{glm::vec3{-1.0f, -1.0f, 0.0f} * scale, glm::vec3{0.0f, 0.0f, 0.0f}, scale}, Game::BLOCK}, 
        {Math::Transform{glm::vec3{-1.0f, -1.0f, 1.0f} * scale, glm::vec3{0.0f, 0.0f, 0.0f}, scale}, Game::BLOCK}, 
        {Math::Transform{glm::vec3{0.0f, -1.0f, 1.0f} * scale, glm::vec3{0.0f, 0.0f, 0.0f}, scale}, Game::BLOCK},
        {Math::Transform{glm::vec3{-1.0f, -1.0f, -1.0f} * scale, glm::vec3{0.0f, 0.0f, 0.0f}, scale}, Game::BLOCK}, 
        {Math::Transform{glm::vec3{0.0f, -1.0f, -1.0f} * scale, glm::vec3{0.0f, 0.0f, 0.0f}, scale}, Game::BLOCK},
        {Math::Transform{glm::vec3{1.0f, -1.0f, -1.0f} * scale, glm::vec3{0.0f, 0.0f, 0.0f}, scale}, Game::BLOCK},
        
        {Math::Transform{glm::vec3{0.0f, 0.0f, 0.0f} * scale, glm::vec3{0.0f, 0.0f, 0.0f}, scale}, Game::SLOPE},
        {Math::Transform{glm::vec3{-1.0f, 0.0f, 0.0f} * scale, glm::vec3{90.0f, 90.0f, 90.0f}, scale}, Game::SLOPE},   
        {Math::Transform{glm::vec3{1.0f, 0.0f, 0.0f} * scale, glm::vec3{0.0f, 0.0f, 90.0f}, scale}, Game::SLOPE},
        {Math::Transform{glm::vec3{0.0f, 0.0f, -1.0f} * scale, glm::vec3{0.0f, 180.0f, 0.0f}, scale}, Game::SLOPE},
    };
}

void PrintVec(glm::vec3 vec, std::string name)
{
    std::cout << name << " is " << vec.x << " " << vec.y << " " << vec.z << '\n';
}

void BlockBuster::Client::Update()
{
    bool clicked = false;
    SDL_Event e;
    while(SDL_PollEvent(&e) != 0)
    {
        ImGui_ImplSDL2_ProcessEvent(&e);
        switch(e.type)
        {
        case  SDL_QUIT:
            quit = true;
            break;
        case SDL_KEYDOWN:
            if(e.key.keysym.sym == SDLK_ESCAPE)
                quit = true;
            if(e.key.keysym.sym == SDLK_g)
            {
                gravity = !gravity;
                std::cout << "Enabled gravity: " << gravity << "\n";
            }
            if(e.key.keysym.sym == SDLK_n)
            {
                noclip = !noclip;
                std::cout << "Toggled noclip : " << noclip << "\n";
            }
            if(e.key.keysym.sym == SDLK_x)
                moveCamera = !moveCamera;
            break;
        case SDL_MOUSEBUTTONDOWN:
            mousePos.x = e.button.x;
            mousePos.y = config.window.resolutionH - e.button.y;
            clicked = true;
            std::cout << "Click coords " << mousePos.x << " " << mousePos.y << "\n";
        }
    }

    // Camera
    if(moveCamera)
    {
        float var = glm::cos(SDL_GetTicks() / 1000.f);
        cameraPos.x = var * 8.0f;
        cameraPos.z = glm::sin(SDL_GetTicks() / 1000.f) * 8.0f;
    }
    Rendering::Camera camera;
    camera.SetPos(cameraPos);
    float pitch = glm::radians(90.0f);
    float yaw = glm::radians(90.0f);
    camera.SetTarget(player.transform.position);
    camera.SetParam(Rendering::Camera::Param::ASPECT_RATIO, (float)config.window.resolutionW / (float)config.window.resolutionH);
    auto rotation = camera.GetRotation();

    // Move Player
    player.Update();
    player.HandleCollisions(blocks);

    // Ray intersection
    if(clicked)
    {
        // Window to eye
        auto ray = Rendering::ScreenToWorldRay(camera, mousePos, glm::vec2{config.window.resolutionW, config.window.resolutionH});

        // Sort blocks by proximity to camera
        auto cameraPos = this->cameraPos;
        std::sort(blocks.begin(), blocks.end(), [cameraPos](Game::Block a, Game::Block b)
        {
            auto toA = glm::length(a.transform.position - cameraPos);
            auto toB = glm::length(b.transform.position- cameraPos);
            return toA < toB;
        });

        // Check intersection
        for(int i = 0; i < blocks.size(); i++)
        {  
            auto model = blocks[i].transform.GetTransformMat();
            auto type = blocks[i].type;

            Collisions::RayIntersection intersection;
            if(type == Game::SLOPE)
                intersection = Collisions::RaySlopeIntersection(ray, model);
            else
                intersection = Collisions::RayAABBIntersection(ray, model);

            if(intersection.intersects)
            {
                auto pos = blocks[i].transform.position;
                auto angle = blocks[i].transform.rotation;

                auto newBlockPos = pos + intersection.normal * scale; 
                auto newBlockType = Game::BLOCK;
                auto newBlockRot = glm::vec3{0.0f};

                blocks.push_back({Math::Transform{newBlockPos, newBlockRot, scale}, newBlockType});

                PrintVec(pos, "Pos");
                PrintVec(newBlockPos, "NewBlockPos");
                break;
            }
        }
    }

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(window_);
    ImGui::NewFrame();
    ImGui::ShowDemoWindow(&showDemoWindow);

    // GUI
    ImGui::Render();
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw cubes/slopes
    for(int i = 0; i < blocks.size(); i++)
    {
        auto model = blocks[i].transform.GetTransformMat();
        auto type = blocks[i].type;
        auto transform = camera.GetProjViewMat() * model;

        shader.SetUniformInt("isPlayer", 0);
        shader.SetUniformMat4("transform", transform);
        if(type == Game::SLOPE)
        {
            slope.Draw(shader, &texture);
        }
        else
        {
            cube.Draw(shader, &gTexture);
        }
    }

    // Draw Player
    shader.SetUniformInt("isPlayer", 1);
    auto transform = camera.GetProjViewMat() * player.transform.GetTransformMat();
    shader.SetUniformMat4("transform", transform);
    cube.Draw(shader, &texture);

    // Draw GUI
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(window_);
}

bool BlockBuster::Client::Quit()
{
    return quit;
}