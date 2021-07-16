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

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

enum BlockType
{
    BLOCK,
    SLOPE
};

struct Block
{
    Math::Transform transform;
    BlockType type;
};

void PrintVec(glm::vec3 vec, std::string name)
{
    std::cout << name << " is " << vec.x << " " << vec.y << " " << vec.z << '\n';
}

Collisions::AABBSlopeIntersection Collisions::AABBSlopeCollision(glm::vec3 posA, glm::vec3 prevPosA, glm::vec3 sizeA, glm::vec3 sizeB, bool gravity, float graivitySpeed, float precision)
{
    auto posB = glm::vec3{0.0f};
    auto delta = posA - prevPosA;

    posA = posA - sizeA * 0.5f;
    prevPosA = prevPosA - sizeA * 0.5f;
    posB = posB - sizeB * 0.5f;
    auto distance = posA - posB;
    auto prevDistance = prevPosA - posB;

    auto boundA = posA + sizeA;
    auto boundB = posB + sizeB;
    auto prevBoundA = prevPosA + sizeA;
    auto diffA = boundB - posA;
    auto diffB = boundA - posB;
    auto prevDiffA = boundB - prevPosA;
    auto prevDiffB = prevBoundA - posB;

    auto prevSign = glm::sign(prevDistance);
    auto sign = glm::sign(distance);
    auto min = glm::min(diffA, diffB);
    auto prevMin = glm::min(prevDiffA, prevDiffB);

    // Checking for collision with slope side
    bool wasAbove = prevSign.y >= 0.0f;
    bool wasInFront = prevSign.z > 0.0f;
    bool wasInSide = prevMin.x > 0.0f && prevMin.x <= (sizeA.x + sizeB.x);
    if(wasInFront && wasAbove)
    {   
        min.y = diffA.z - (posA.y - posB.y);
        min.z = diffA.y - (posA.z - posB.z);

        // Dealing with floating point precision
        min.y = (float)round(min.y / precision) * precision;
        min.z = (float)round(min.z / precision) * precision;

        min.y *= 0.5f;
        min.z *= 0.5f;

        sign.y = 1.0f;
        sign.z = 1.0f;

        if(wasInSide && gravity)
        {
            min.x += (-graivitySpeed);
            std::cout << "Increasing size\n";
        }
    }

    // Collision detection
    auto collision = glm::greaterThanEqual(min, glm::vec3{0.0f}) && glm::lessThan(min, glm::vec3{sizeA + sizeB});
    auto intersects = collision.x && collision.y && collision.z;

    // Offset and normal calculation
    auto minAxis = glm::step(min, glm::vec3{min.z, min.x, min.y}) * glm::step(min, glm::vec3{min.y, min.z, min.x});
    auto normal = sign * minAxis;
    auto collidedFront = prevMin.z > 0.0f && prevMin.z <= (sizeA.z + sizeB.z);

    auto offset = min * normal;
    PrintVec(min, "Min");

    return AABBSlopeIntersection{intersects, offset, normal, min, minAxis, sign};
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
    GL::Texture texture = GL::Texture::FromFolder(TEXTURES_DIR, "SmoothStone.png");
    GL::Texture gTexture = GL::Texture::FromFolder(TEXTURES_DIR, "green.png");
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
    glm::vec3 playerPos{0.0f, -0.5f, 4.0f};
    float scale = 4.0f;

    std::vector<Block> blocks{
        {Math::Transform{glm::vec3{1.0f, -1.0f, 0.0f} * scale, glm::vec3{0.0f, 0.0f, 0.0f}, scale}, BLOCK}, 
        {Math::Transform{glm::vec3{1.0f, -1.0f, 1.0f} * scale, glm::vec3{0.0f, 0.0f, 0.0f}, scale}, BLOCK},
        {Math::Transform{glm::vec3{-1.0f, -1.0f, 0.0f} * scale, glm::vec3{0.0f, 0.0f, 0.0f}, scale}, BLOCK}, 
        {Math::Transform{glm::vec3{-1.0f, -1.0f, 1.0f} * scale, glm::vec3{0.0f, 0.0f, 0.0f}, scale}, BLOCK}, 
        {Math::Transform{glm::vec3{0.0f, -1.0f, 1.0f} * scale, glm::vec3{0.0f, 0.0f, 0.0f}, scale}, BLOCK},
        {Math::Transform{glm::vec3{-1.0f, 0.0f, 0.0f} * scale, glm::vec3{0.0f, 90.0f, 90.0f}, scale}, SLOPE},   
        {Math::Transform{glm::vec3{0.0f, 0.0f, 0.0f} * scale, glm::vec3{0.0f, 0.0f, 0.0f}, scale}, SLOPE},
        {Math::Transform{glm::vec3{1.0f, 0.0f, 0.0f} * scale, glm::vec3{0.0f, 0.0f, 90.0f}, scale}, SLOPE},
        //{Math::Transform{glm::vec3{0.0f, 0.0f, -1.0f} * scale, glm::vec3{0.0f, 180.0f, 0.0f}, scale}, SLOPE},
        {Math::Transform{glm::vec3{-1.0f, -1.0f, -1.0f} * scale, glm::vec3{0.0f, 0.0f, 0.0f}, scale}, BLOCK}, 
        {Math::Transform{glm::vec3{0.0f, -1.0f, -1.0f} * scale, glm::vec3{0.0f, 0.0f, 0.0f}, scale}, BLOCK},
        {Math::Transform{glm::vec3{1.0f, -1.0f, -1.0f} * scale, glm::vec3{0.0f, 0.0f, 0.0f}, scale}, BLOCK}, 
        
    };
    bool gravity = false;
    const float gravitySpeed = -0.4f;
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
        
        std::cout << "Gravity: " << gravity << "\n";
        auto prevPos = playerPos;
        playerPos = playerPos + moveDir * speed;
        bool gravityAffected = gravity;
        if(gravity)
        {
            playerPos = playerPos + glm::vec3{0.0f, gravitySpeed, 0.0f};
            PrintVec(playerPos, "PostGravity");
        }
        gravity = true;

        PrintVec(playerPos, "Precollison");
        for(auto block : blocks)
        {
            auto slopePos = block.transform.position;
            auto rotation = block.transform.GetRotationMat();

            if(block.type == SLOPE)
            {
                // Collision player and slope i                
                glm::mat4 translation = glm::translate(glm::mat4{1.0f}, slopePos);
                auto transform = translation * rotation;
                glm::vec3 posA = glm::inverse(transform) * glm::vec4{playerPos, 1.0f};
                glm::vec3 prevPosA = glm::inverse(transform) * glm::vec4{prevPos, 1.0f};
                auto slopeIntersect = Collisions::AABBSlopeCollision(posA, prevPosA, glm::vec3{1.0f}, glm::vec3{scale}, true, gravitySpeed);
                if(slopeIntersect.intersects)
                {
                    PrintVec(slopeIntersect.offset, "Offset");
                    PrintVec(slopeIntersect.normal, "Normal");
                    glm::vec3 offset = rotation * glm::vec4{slopeIntersect.offset, 1.0f};
                    glm::vec3 normal = rotation * glm::vec4{slopeIntersect.normal, 1.0f};

                    if (normal.y > 0.0f && slopeIntersect.normal.z > 0.0f && slopeIntersect.normal.y > 0.0f)
                    {
                       offset.z = 0.0f;
                       offset.y *= 2.0f;
                    }
                    else if (normal.y > 0.0f)
                    {
                        gravity = false;
                    }

                    playerPos = playerPos + offset;
                }

                /*
                auto boxIntersect = Collisions::AABBCollision(playerPos, glm::vec3{1.0f}, slopePos, glm::vec3{block.transform.scale});
                if(boxIntersect.intersects)
                {
                    playerPos = playerPos + boxIntersect.offset;

                    auto isGround = boxIntersect.normal.y > 0.0f && glm::abs(boxIntersect.normal.x) == 0.0f && glm::abs(boxIntersect.normal.z) == 0.0f;
                    if(isGround)
                        gravity = false;
                }
                */
            }
            else
            {
                auto boxIntersect = Collisions::AABBCollision(playerPos, glm::vec3{1.0f}, slopePos, glm::vec3{block.transform.scale});
                if(boxIntersect.intersects)
                {
                    playerPos = playerPos + boxIntersect.offset;

                    auto isGround = boxIntersect.normal.y > 0.0f && glm::abs(boxIntersect.normal.x) == 0.0f && glm::abs(boxIntersect.normal.z) == 0.0f;
                    if(isGround)
                        gravity = false;
                }
            }
        }
        PrintVec(playerPos, "Postcollison");
        std::cout << "\n";

        // Player and blocks collision
        std::sort(blocks.begin(), blocks.end(), [cameraPos](Block a, Block b)
        {
            auto toA = glm::length(a.transform.position - cameraPos);
            auto toB = glm::length(b.transform.position- cameraPos);
            return toA < toB;
        });

        // Ray intersection
        if(clicked)
        {
            // Window to eye
            auto ray = Rendering::ScreenToWorldRay(camera, mousePos, glm::vec2{WINDOW_WIDTH, WINDOW_HEIGHT});

            // Check intersection
            for(int i = 0; i < blocks.size(); i++)
            {  

                //auto model = glm::translate(glm::mat4{1.0f}, blocks[i].pos * boxSize);
                auto model = blocks[i].transform.GetTransformMat();
                auto type = blocks[i].type;

                Collisions::RayIntersection intersection;
                if(type == SLOPE)
                    intersection = Collisions::RaySlopeIntersection(ray, model);
                else
                    intersection = Collisions::RayAABBIntersection(ray, model);

                if(intersection.intersects)
                {
                    auto pos = blocks[i].transform.position;
                    auto angle = blocks[i].transform.rotation;

                    auto newBlockPos = pos + intersection.normal * scale; 
                    auto newBlockType = BLOCK;
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
            if(type == SLOPE)
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
        auto playerTransform = Math::Transform{playerPos, glm::vec3{0.0f}, 1.0f};
        auto transform = camera.GetProjViewMat() * playerTransform.GetTransformMat();
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
