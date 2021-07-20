#include <glad/glad.h>

#include <glm/gtc/matrix_transform.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/backends/imgui_impl_sdl.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <iostream>
#include <algorithm>

#include <gl/Shader.h>
#include <gl/VertexArray.h>
#include <gl/Texture.h>

#include <rendering/Camera.h>
#include <rendering/Mesh.h>
#include <rendering/Primitive.h>
#include <rendering/Rendering.h>

#include <collisions/Collisions.h>

#include <math/Transform.h>

#include <game/Block.h>
#include <game/Player.h>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

void PrintVec(glm::vec3 vec, std::string name)
{
    std::cout << name << " is " << vec.x << " " << vec.y << " " << vec.z << '\n';
}

float FixFloat(float a, float precision)
{
    return (float)round(a / precision) * precision;
}

int main()
{
    if(SDL_Init(0))
    {
        std::cout << "SDL Init failed: " << SDL_GetError() << "\n";
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    auto window = SDL_CreateWindow("SDL Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
    if(!window)
    {
        std::cout << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        return -1;
    }
    auto context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, context);

    if(!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        std::cout << "gladLoadGLLoader failed \n";
        return -1;
    }
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    GL::Shader shader = GL::Shader::FromFolder(SHADERS_DIR, "vertex.glsl", "fragment.glsl");
    shader.Use();

    auto cube = Rendering::Primitive::GenerateCube();
    GL::Texture texture = GL::Texture::FromFolder(TEXTURES_DIR, "green.png");
    GL::Texture gTexture = GL::Texture::FromFolder(TEXTURES_DIR, "SmoothStone.png");
    try
    {
        texture.Load();
        gTexture.Load();
    }
    catch(const GL::Texture::LoadError& e)
    {
        std::cout << "Error when loading texture " + e.path_.string() + ": " +  e.what() << '\n';
    }

    auto slope = Rendering::Primitive::GenerateSlope();
    
    glEnable(GL_DEPTH_TEST);

    // ImGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForOpenGL(window, context);
    ImGui_ImplOpenGL3_Init("#version 330");

    bool showDemoWindow = true;
    bool quit = false;

    glm::vec2 mousePos;
    float scale = 5.0f;
    float playerScale = 2.0f;
    AppGame::Player player;
    player.transform.scale = playerScale;

    std::vector<Game::Block> blocks{
        {Math::Transform{glm::vec3{1.0f, -1.0f, 0.0f} * scale, glm::vec3{0.0f, 0.0f, 0.0f}, scale}, Game::BLOCK}, 
        {Math::Transform{glm::vec3{1.0f, -1.0f, 1.0f} * scale, glm::vec3{0.0f, 0.0f, 0.0f}, scale}, Game::BLOCK},
        {Math::Transform{glm::vec3{-1.0f, -1.0f, 0.0f} * scale, glm::vec3{0.0f, 0.0f, 0.0f}, scale}, Game::BLOCK}, 
        {Math::Transform{glm::vec3{-1.0f, -1.0f, 1.0f} * scale, glm::vec3{0.0f, 0.0f, 0.0f}, scale}, Game::BLOCK}, 
        {Math::Transform{glm::vec3{0.0f, -1.0f, 1.0f} * scale, glm::vec3{0.0f, 0.0f, 0.0f}, scale}, Game::BLOCK},
        {Math::Transform{glm::vec3{-1.0f, -1.0f, -1.0f} * scale, glm::vec3{0.0f, 0.0f, 0.0f}, scale}, Game::BLOCK}, 
        {Math::Transform{glm::vec3{0.0f, -1.0f, -1.0f} * scale, glm::vec3{0.0f, 0.0f, 0.0f}, scale}, Game::BLOCK},
        {Math::Transform{glm::vec3{1.0f, -1.0f, -1.0f} * scale, glm::vec3{0.0f, 0.0f, 0.0f}, scale}, Game::BLOCK},
        
        {Math::Transform{glm::vec3{0.0f, 0.0f, 0.0f} * scale, glm::vec3{0.0f, 0.0f, 0.0f}, scale}, Game::SLOPE, "Mid"},
        {Math::Transform{glm::vec3{-1.0f, 0.0f, 0.0f} * scale, glm::vec3{90.0f, 90.0f, 90.0f}, scale}, Game::SLOPE, "Left"},   
        {Math::Transform{glm::vec3{1.0f, 0.0f, 0.0f} * scale, glm::vec3{0.0f, 0.0f, 90.0f}, scale}, Game::SLOPE, "Right"},
        {Math::Transform{glm::vec3{0.0f, 0.0f, -1.0f} * scale, glm::vec3{0.0f, 180.0f, 0.0f}, scale}, Game::SLOPE, "Back"},
        
        
    };
    bool gravity = false;
    const float gravitySpeed = -0.4f;
    bool noclip = false;
    uint frame = 0;
    
    bool moveCamera = false;
    glm::vec3 cameraPos{0.0f, 12.0f, 8.0f};
    while(!quit)
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
                mousePos.y = WINDOW_HEIGHT - e.button.y;
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
        camera.SetParam(Rendering::Camera::Param::ASPECT_RATIO, (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT);
        auto rotation = camera.GetRotation();

        // Move Player
        player.Update();
        player.HandleCollisions(blocks);

        // Ray intersection
        if(clicked)
        {
            // Sort blocks by proximity to camera
            std::sort(blocks.begin(), blocks.end(), [cameraPos](Game::Block a, Game::Block b)
            {
                auto toA = glm::length(a.transform.position - cameraPos);
                auto toB = glm::length(b.transform.position- cameraPos);
                return toA < toB;
            });

            // Window to eye
            auto ray = Rendering::ScreenToWorldRay(camera, mousePos, glm::vec2{WINDOW_WIDTH, WINDOW_HEIGHT});

            // Check intersection
            for(int i = 0; i < blocks.size(); i++)
            {  

                //auto model = glm::translate(glm::mat4{1.0f}, blocks[i].pos * boxSize);
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
        ImGui_ImplSDL2_NewFrame(window);
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

        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}
