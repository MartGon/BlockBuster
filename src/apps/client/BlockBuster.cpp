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

#include <collisions/Collisions.h>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

enum BlockType
{
    BLOCK,
    SLOPE
};

struct Block
{
    glm::vec3 pos;
    glm::vec3 rot;
    BlockType type;
};

void PrintVec(glm::vec3 vec, std::string name)
{
    std::cout << name << " is " << vec.x << " " << vec.y << " " << vec.z << '\n';
}

float FixFloat(float a, float precision)
{
    return (float)round(a / precision) * precision;
}

struct Ray
{
    glm::vec3 origin;
    glm::vec3 destiny;
};

Ray MyScreenToWorld(glm::vec2 mousePos, glm::mat4 projViewMat)
{
    glm::vec3 windowPos{mousePos.x, mousePos.y, 100.0f};
    glm::vec4 nd{(windowPos.x * 2.0f) / (float)WINDOW_WIDTH - 1.0f, 
                (windowPos.y * 2.0f) / (float) WINDOW_HEIGHT - 1.0f, 
                (2 * windowPos.z - 100.0f - 0.1f) / (100.f - 0.1f), 1.0f};
    nd.z = -1.0f;
    glm::vec4 clipPos = nd / 1.0f;
    glm::mat4 screenToWorld = glm::inverse(projViewMat);
    glm::vec4 worldPos = screenToWorld * clipPos;
    worldPos = worldPos / worldPos.w;

    return Ray{glm::vec3{0.0f}, worldPos};
}

Ray ScreenToWorld(glm::vec2 mousePos, glm::mat4 projViewMat)
{
    glm::vec4 rayStartND{(mousePos.x * 2.0f) / (float)WINDOW_WIDTH - 1.0f, 
                (mousePos.y * 2.0f) / (float) WINDOW_HEIGHT - 1.0f, 
                -1.0f, 1.0f};
    glm::vec4 rayEndND{(mousePos.x * 2.0f) / (float)WINDOW_WIDTH - 1.0f, 
                (mousePos.y * 2.0f) / (float) WINDOW_HEIGHT - 1.0f, 
                0.0f, 1.0f};
        
    glm::mat4 screenToWorld = glm::inverse(projViewMat);
    glm::vec4 rayStartWorld = screenToWorld * rayStartND; rayStartWorld /= rayStartWorld.w;
    glm::vec4 rayEndWorld = screenToWorld * rayEndND;   rayEndWorld /= rayEndWorld.w;

    glm::vec3 rayDir = rayEndWorld - rayStartWorld;
    //rayDir = glm::normalize(rayDir);

    return Ray{rayStartWorld, rayEndWorld};
}

Rendering::Mesh GenerateCube()
{
    Rendering::Mesh cube;
    GL::VertexArray& cubeVao = cube.GetVAO();
    cubeVao.GenVBO(std::vector<float>
    {   
        // Down
        -0.5, -0.5, -0.5,       
        -0.5, -0.5, 0.5,        
        0.5, -0.5, -0.5,        
        0.5, -0.5, 0.5,         

        // Up
        -0.5, 0.5, -0.5,        
        -0.5, 0.5, 0.5,         
        0.5, 0.5, -0.5,         
        0.5, 0.5, 0.5,          

        // Front
        -0.5, 0.5, 0.5,         
        -0.5, -0.5, 0.5,        
        0.5, 0.5, 0.5,          
        0.5, -0.5, 0.5,         

        // Back
        -0.5, 0.5, -0.5,        
        -0.5, -0.5, -0.5,       
        0.5, 0.5, -0.5,         
        0.5, -0.5, -0.5,        

        // Left
        -0.5, -0.5, -0.5,      
        -0.5, 0.5, -0.5,       
        -0.5, -0.5, 0.5,       
        -0.5, 0.5, 0.5,        

        // Right
        0.5, -0.5, -0.5,      
        0.5, 0.5, -0.5,       
        0.5, -0.5, 0.5,       
        0.5, 0.5, 0.5,        
    }, 3);
    cubeVao.GenVBO(std::vector<float>{
        // Down
        0.0, -1.0, 0.0,
        0.0, -1.0, 0.0,
        0.0, -1.0, 0.0,
        0.0, -1.0, 0.0,

        // Up
        0.0, 1.0, 0.0, 
        0.0, 1.0, 0.0, 
        0.0, 1.0, 0.0, 
        0.0, 1.0, 0.0, 

        // Front
        0.0, 0.0, 1.0, 
        0.0, 0.0, 1.0, 
        0.0, 0.0, 1.0, 
        0.0, 0.0, 1.0, 

        // Back
        0.0, 0.0, -1.0,
        0.0, 0.0, -1.0,
        0.0, 0.0, -1.0,
        0.0, 0.0, -1.0,

        // Left
        1.0, 0.0, 0.0, 
        -1.0, 0.0, 0.0,
        -1.0, 0.0, 0.0,
        -1.0, 0.0, 0.0,

        // Right
        1.0, 0.0, 0.0,  
        -1.0, 0.0, 0.0,  
        -1.0, 0.0, 0.0,  
        -1.0, 0.0, 0.0,                            
    }, 3);
    cubeVao.GenVBO(std::vector<float>{
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,

        1.0f, 1.0f,
        0.0f, 0.0f,
        0.0f, 1.0f,

        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,

        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,

        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,

        1.0f, 1.0f,
        0.0f, 0.0f,
        0.0f, 1.0f,

        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,

        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
    }, 2);
    cubeVao.SetIndices({
        // Down
        0, 1, 2,
        1, 2, 3,

        // Up
        4, 5, 6,
        5, 6, 7,

        // Front
        8, 9, 10,
        9, 10, 11,

        // Back
        12, 13, 14,
        13, 14, 15,

        // Right
        16, 17, 18,
        17, 18, 19,

        // Left
        20, 21, 22,
        21, 22, 23,

    });

    return cube;
}

Rendering::Mesh GenerateSlope()
{
    Rendering::Mesh slope;
    GL::VertexArray& slopeVao = slope.GetVAO();
    slopeVao.GenVBO(std::vector<float>
    {
        // Base
        -0.5, -0.5, 0.5,
        0.5, -0.5, 0.5,
        -0.5, -0.5, -0.5,
        0.5, -0.5, -0.5,

        -0.5, 0.5, -0.5,
        0.5, 0.5, -0.5
    }, 3);
    slopeVao.SetIndices({
        // Base
        0, 1, 2,
        1, 2, 3,

        // Left
        0, 2, 4,

        // Right
        1, 3, 5,

        // Back
        2, 3, 4,
        3, 4, 5,

        // Ramp
        0, 1, 4,
        1, 4, 5
    });

    return slope;
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

    std::filesystem::path shadersDir{SHADERS_DIR};
    std::filesystem::path vertex = shadersDir / "vertex.glsl";
    std::filesystem::path fragment = shadersDir / "fragment.glsl";
    GL::Shader shader{vertex, fragment};
    shader.Use();

    auto cube = GenerateCube();
    std::filesystem::path textureFolder{TEXTURES_DIR};
    std::filesystem::path texturePath = textureFolder / "SmoothStone.png";
    GL::Texture texture{texturePath};
    try
    {
        std::cout << "Hello!\n";
        texture.Load();
    }
    catch(const GL::Texture::LoadError& e)
    {
        std::cout << "Error when loading texture " + e.path_.string() + ": " +  e.what() << '\n';
    }
    cube.SetTexture(&texture);

    auto slope = GenerateSlope();
    
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

    glm::vec3 playerPos{-1.5f, -0.5f, 0.0f};
    glm::vec3 boxSize{2.f};
    auto cubePos = glm::vec3{1.0f, 0.0f, 0.0f};

    std::vector<Block> blocks{
        {glm::vec3{-1.0f, 0.0f, 0.0f}, glm::vec3{0.0f, 0.0f, 0.0f}, BLOCK},
        
        {glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.0f, 0.0f, 0.0f}, SLOPE},
        {glm::vec3{1.0f, 0.0f, 0.0f}, glm::vec3{0.0f, 0.0f, 90.0f}, BLOCK},
        {glm::vec3{0.0f, 0.0f, -1.0f}, glm::vec3{0.0f, 180.0f, 0.0f}, SLOPE},
        
    };
    bool gravity = false;
    bool noclip = false;
    bool isOnSlope = false;
    uint frame = 0;
    
    bool moveCamera = false;
    glm::vec3 cameraPos{0.0f, 12.0f, 8.0f};
    while(!quit)
    {
        bool clicked = false;
        bool grounded = false;
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
        camera.SetTarget(playerPos);
        camera.SetParam(Rendering::Camera::Param::ASPECT_RATIO, (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT);
        auto rotation = camera.GetRotation();
        // Move Player
        auto state = SDL_GetKeyboardState(nullptr);
        float speed = 0.05f;
        float gravitySpeed = -0.1f;

        glm::vec3 moveDir{0};
        if(state[SDL_SCANCODE_A])
            moveDir.x -= 1;
        if(state[SDL_SCANCODE_D])
            moveDir.x += 1;
        if(state[SDL_SCANCODE_W])
            moveDir.z -= 1;
        if(state[SDL_SCANCODE_S])
            moveDir.z += 1;
        if(state[SDL_SCANCODE_Q])
            moveDir.y += 1;
        if(state[SDL_SCANCODE_E])
            moveDir.y -= 1;
        if(state[SDL_SCANCODE_F])
            playerPos = glm::vec3{0.0f, 2.0f, 0.0f};
        
        playerPos = playerPos + moveDir * speed;
        if(isOnSlope)
            gravity = true;
        if(gravity)
            playerPos = playerPos + glm::vec3{0.0f, gravitySpeed, 0.0f};

        // Player and blocks collision
        std::sort(blocks.begin(), blocks.end(), [cameraPos](Block a, Block b)
        {
            auto toA = glm::length(a.pos - cameraPos);
            auto toB = glm::length(b.pos- cameraPos);
            return toA < toB;
        });

        std::vector<glm::mat4> models;
        isOnSlope = false;
        for(auto block : blocks)
        {
            auto slopePos = block.pos * boxSize;
            auto angle = block.rot;

            auto rotation = glm::rotate(glm::mat4{1.0f}, glm::radians(angle.y), glm::vec3{0.0f, 1.0f, 0.0f});
            rotation = glm::rotate(rotation, glm::radians(0.0f), glm::vec3{1.0f, 0.0f, 0.0f});
            rotation = glm::rotate(rotation, glm::radians(angle.z), glm::vec3{0.0f, 0.0f, 1.0f});

            if(block.type == SLOPE)
            {
                 // Collision player and slope i                
                glm::mat4 translation = glm::translate(glm::mat4{1.0f}, slopePos);
                auto transform = translation * rotation;
                auto posA = glm::inverse(transform) * glm::vec4{playerPos, 1.0f};
                auto boxIntersect = Collisions::AABBSlopeCollision(posA, glm::vec3{1.0f}, boxSize);
                if(boxIntersect.intersects)
                {
                    // Collision resolution in model space
                    glm::vec3 offset = rotation * glm::vec4{boxIntersect.offset, 1.0f};
                    //PrintVec(boxIntersect.normal, "Normal");

                    // Check for slope orientation and fix offset is model space
                    if (boxIntersect.normal.z > 0.0f && boxIntersect.normal.y > 0.0f)
                    {
                        offset.z = 0.0f;
                        offset.y *= 2.0f;
                        isOnSlope = true;
                    }
                    
                    auto normal = rotation * glm::vec4{boxIntersect.normal, 1.0f};
                    auto isGround = normal.y > 0.0f;
                    if(isGround)
                        grounded = true;                

                    if(!noclip)
                        playerPos = playerPos + offset;
                }
            }
            else
            {
                auto boxIntersect = Collisions::AABBCollision(playerPos, glm::vec3{1.0f}, slopePos, boxSize);
                if(boxIntersect.intersects)
                {
                    playerPos = playerPos + boxIntersect.offset;

                    auto isGround = boxIntersect.normal.y > 0.0f && glm::abs(boxIntersect.normal.x) == 0.0f && glm::abs(boxIntersect.normal.z) == 0.0f;
                    if(isGround)
                        grounded = true; 
                }
            }

            // Calculate cube/slope model matrix
            glm::mat4 model{1.0f};
            model = glm::translate(model, slopePos);
            model = glm::scale(model, boxSize);
            model = model * rotation;
            models.push_back(model);
        }

        // Gravity
        if(noclip || grounded)
            gravity = false;
        else
            gravity = true;

        // Player transfrom
        glm::mat4 playerModel = glm::translate(glm::mat4{1.0f}, playerPos);
        auto playerTransform = camera.GetProjViewMat() * playerModel;

        // Ray intersection
        if(clicked)
        {
            // Window to eye
            auto ray = MyScreenToWorld(mousePos, camera.GetProjViewMat());

            // Check intersection
            for(int i = 0; i < models.size(); i++)
            {  

                //auto model = glm::translate(glm::mat4{1.0f}, blocks[i].pos * boxSize);
                auto model = models[i];
                
                glm::mat4 worldToModel = glm::inverse(model);
                glm::vec3 modelRayOrigin = glm::vec3{worldToModel * glm::vec4(cameraPos, 1.0f)};
                glm::vec3 modelRayDest = worldToModel * glm::vec4{ray.destiny, 1.0f};
                glm::vec3 rayDir = glm::normalize(modelRayDest - modelRayOrigin);
                PrintVec(modelRayOrigin, "Origin");
                PrintVec(modelRayDest, "Deset");
                PrintVec(rayDir, "Dir");
                
                auto type = blocks[i].type;
                Collisions::RayIntersection intersection;
                switch (type)
                {
                case SLOPE:
                    {
                        intersection = Collisions::RaySlopeIntersection(modelRayOrigin, rayDir, glm::vec3{0.5f});
                        break;
                    }
                default:
                    {
                        intersection = Collisions::RayAABBIntersection(modelRayOrigin, rayDir, glm::vec3{0.5f});
                        break;
                    }
                }

                if(intersection.intersects)
                {
                    auto pos = blocks[i].pos;
                    auto angle = blocks[i].rot;
                    auto rotation = glm::rotate(glm::mat4{1.0f}, glm::radians(angle.y), glm::vec3{0.0f, 1.0f, 0.0f});
                    rotation = glm::rotate(rotation, glm::radians(0.0f), glm::vec3{1.0f, 0.0f, 0.0f});
                    rotation = glm::rotate(rotation, glm::radians(angle.z), glm::vec3{0.0f, 0.0f, 1.0f});                    
                    glm::vec3 normal = rotation * glm::vec4{intersection.normal, 1.0f};

                    auto newBlockPos = pos + normal;
                    auto newBlockType = BLOCK;
                    auto newBlockRot = glm::vec3{0.0f};

                    blocks.push_back({newBlockPos, newBlockRot, newBlockType});

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
        for(int i = 0; i < models.size(); i++)
        {
            auto model = models[i];
            auto type = blocks[i].type;
            auto transform = camera.GetProjViewMat() * model;

            shader.SetUniformInt("isPlayer", 0);
            shader.SetUniformMat4("transform", transform);
            if(type == SLOPE)
            {
                slope.Draw(shader);
            }
            else
            {
                cube.Draw(shader);
            }

        }

        // Draw Player
        shader.SetUniformInt("isPlayer", 1);
        shader.SetUniformMat4("transform", playerTransform);
        cube.Draw(shader);

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
