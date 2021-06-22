#include <glad/glad.h>

#include <glm/gtc/matrix_transform.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/backends/imgui_impl_sdl.h>

#include <iostream>

#include <gl/Shader.h>
#include <gl/VertexArray.h>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

struct RayIntersection
{
    bool intersects;
    glm::vec2 ts;
    glm::vec3 normal;
};

struct AABBIntersection
{
    bool intersects;
    glm::vec3 offset;
    glm::vec3 min;
    glm::vec3 normal;
};

void PrintVec(glm::vec3 vec, std::string name)
{
    std::cout << name << " is " << vec.x << " " << vec.y << " " << vec.z << '\n';
}

float FixFloat(float a, float precision)
{
    return (float)round(a / precision) * precision;
}

RayIntersection RayAABBIntersection(glm::vec3 rayOrigin, glm::vec3 rayDir, glm::vec3 boxSize)
{
    glm::vec3 m = 1.0f / rayDir;
    glm::vec3 n = m * rayOrigin;
    glm::vec3 k = glm::abs(m) * boxSize;
    auto t1 = -n - k;
    auto t2 = -n + k;
    float tN = glm::max(glm::max(t1.x, t1.y), t1.z);
    float tF = glm::min(glm::min(t2.x, t2.y), t2.z);

    bool intersection = !(tN > tF);
    //std::cout << "TN: " << tN << " TF: " << tF << '\n';
    std::cout << "Ray intersection: " << intersection << '\n';

    //std::cout << "T1 is " << t1.x << " " << t1.y << " " << t1.z << '\n';
    //std::cout << "T1.yzx is " << t1.y << " " << t1.z << " " << t1.x << '\n';
    //std::cout << "T1.zyx is " << t1.z << " " << t1.x << " " << t1.y << '\n';
    auto step1 = glm::step(glm::vec3{t1.y, t1.z, t1.x}, t1);
    //std::cout << "Step 1 is " << step1.x << " " << step1.y << " " << step1.z << '\n';
    auto step2 = glm::step(glm::vec3{t1.z, t1.x, t1.y}, t1);
    //std::cout << "Step 2 is " << step2.x << " " << step2.y << " " << step2.z << '\n';
    auto normal = -glm::sign(rayDir) * step1 * step2;
    //std::cout << "Normal is " << normal.x << " " << normal.y << " " << normal.z << '\n';

    glm::vec3 myNormal;
    if (tN == t1.x)
        myNormal = {1.0f, 0.0f, 0.0f};
    else if(tN == t1.y)
        myNormal = {0.0f, 1.0f, 0.0f};
    else
        myNormal = {0.0f, 0.0f, 1.0f};

    myNormal = -glm::sign(rayDir) * myNormal;
    std::cout << "My normal is " << myNormal.x << " " << myNormal.y << " " << myNormal.z << '\n';

    return RayIntersection{intersection, glm::vec2{tN, tF}, normal};
}

bool RaySlopeIntersectionCheckPoint(glm::vec3 point)
{   
    bool intersects = false;
    auto min = glm::sign(point) * glm::step(glm::abs(glm::vec3{point.y, point.z, point.x}), glm::abs(point)) 
        * glm::step(glm::abs(glm::vec3{point.z, point.x, point.y}), glm::abs(point));

    if(min.z == -1 || min.y == -1)
    {
        intersects = true;
    }

    if(min.x == 1 || min.x == -1)
    {        
        intersects = point.y <= (-1 * point.z);
    }

    return intersects;
}

RayIntersection RaySlopeIntersection(glm::vec3 rayOrigin, glm::vec3 rayDir, glm::vec3 boxSize)
{
    auto aabbIntersect = RayAABBIntersection(rayOrigin, rayDir, boxSize);
    RayIntersection intersection{false, aabbIntersect.ts, aabbIntersect.normal};
    if(aabbIntersect.intersects)
    {
        auto normal = aabbIntersect.normal;
        auto nearPoint = rayOrigin + rayDir * aabbIntersect.ts.x;
        auto farPoint = rayOrigin + rayDir * aabbIntersect.ts.y;
        PrintVec(nearPoint + glm::vec3{0.5f}, "Near Point");
        PrintVec(farPoint + glm::vec3{0.5f}, "Far Point");

        auto nearInt = RaySlopeIntersectionCheckPoint(nearPoint);
        auto farInt = RaySlopeIntersectionCheckPoint(farPoint);
        std::cout << "NearInt: " << nearInt << "\n";
        std::cout << "FarInt: " << farInt << "\n";
        intersection.intersects = nearInt || farInt;
    }

    return intersection;
}

AABBIntersection AABBCollision(glm::vec3 posA, glm::vec3 sizeA, glm::vec3 posB, glm::vec3 sizeB)
{
    // Move to down-left-back corner
    posA = posA - sizeA * 0.5f;
    posB = posB - sizeB * 0.5f;

    auto boundA = posA + sizeA;
    auto boundB = posB + sizeB;
    auto diffA = boundB - posA;
    auto diffB = boundA - posB;

    //PrintVec(diffA, "DiffA");
    //PrintVec(diffB, "diffB");

    auto sign = glm::sign(posA - posB);
    auto min = glm::min(diffA, diffB);
    auto minAxis = glm::step(min, glm::vec3{min.z, min.x, min.y}) * glm::step(min, glm::vec3{min.y, min.z, min.x});
    auto normal = minAxis * sign;
    auto offset = sign * min * minAxis;

    auto collision = glm::greaterThan(min, glm::vec3{0.0f}) && glm::lessThan(min, glm::vec3{sizeA + sizeB});
    bool intersects = collision.x && collision.y && collision.z;

    return AABBIntersection{intersects, offset, min, normal};
}

AABBIntersection AABBSlopeCollision(glm::vec3 posA, glm::vec3 sizeA, glm::vec3 sizeB, float precision = 0.05f)
{
    auto posB = glm::vec3{0.0f};

    posA = posA - sizeA * 0.5f;
    posB = posB - sizeB * 0.5f;

    auto boundA = posA + sizeA;
    auto boundB = posB + sizeB;
    auto diffA = boundB - posA;
    auto diffB = boundA - posB;

    auto distance = posA - posB;
    auto sign = glm::sign(distance);
    auto min = glm::min(diffA, diffB);

    // Checking for collision with slope side
    bool isAbove = sign.y >= 0.0f;
    bool isInFront = sign.z > 0.0f;
    if(isInFront && isAbove)
    {   
        min.y = diffA.z - (posA.y - posB.y);
        min.z = diffA.y - (posA.z - posB.z);

        // Dealing with floating point precision
        min.y = (float)round(min.y / precision) * precision;
        min.z = (float)round(min.z / precision) * precision;

        // This makes it so it's pushed along the slope normal;
        min.y *= 0.5f;
        min.z *= 0.5f;
    }

    // Collision detection
    auto collision = glm::greaterThanEqual(min, glm::vec3{0.0f}) && glm::lessThan(min, glm::vec3{sizeA + sizeB});
    auto intersects = collision.x && collision.y && collision.z;

    // Offset and normal calculation
    auto minAxis = glm::step(min, glm::vec3{min.z, min.x, min.y}) * glm::step(min, glm::vec3{min.y, min.z, min.x});
    auto normal = sign * minAxis;
    auto offset = sign * min * minAxis;

    return AABBIntersection{intersects, offset, min, normal};
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

    GL::VertexArray vao;
    vao.GenVBO(std::vector<float>
    {
        -0.5, -0.5, -0.5,
        0.5, -0.5, -0.5,
        0.5, 0.5, -0.5,
        -0.5, 0.5, -0.5,
        
        -0.5, -0.5, 0.5,
        0.5, -0.5, 0.5,
        0.5, 0.5, 0.5,
        -0.5, 0.5, 0.5,
    });
    vao.SetIndices({
                    // Front Face
                    0, 1, 2,
                    3, 0, 2,

                    // Back face
                    4, 5, 6,
                    7, 4, 6,

                    // Left Face
                    0, 4, 3,
                    7, 4, 3,

                    // Right face
                    1, 2, 5,
                    6, 2, 5,

                    // Up face
                    2, 3, 6,
                    7, 3, 6,

                    // Down face
                    0, 1, 4,
                    5, 1, 4
                    });
    vao.AttribPointer(0, 3, GL_FLOAT, false, 0);

    GL::VertexArray slopeVao;
    slopeVao.GenVBO(std::vector<float>
    {
        // Base
        -0.5, -0.5, 0.5,
        0.5, -0.5, 0.5,
        -0.5, -0.5, -0.5,
        0.5, -0.5, -0.5,

        -0.5, 0.5, -0.5,
        0.5, 0.5, -0.5
    });
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
    slopeVao.AttribPointer(0, 3, GL_FLOAT, false, 0);
    
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
    glm::vec3 boxSize{3.0f};
    auto cubePos = glm::vec3{1.0f, 0.0f, 0.0f};

    std::vector<glm::vec3> slopesPos{
        glm::vec3{-1.0f, 0.0f, 0.0f},
        glm::vec3{0.0f, 0.0f, 0.0f},
        glm::vec3{1.0f, 0.0f, 0.0f},
        glm::vec3{0.0f, 0.0f, -1.0f},
    };
    std::vector<glm::vec3> slopesRotation{
        glm::vec3{0.0f, 0.0f, -90.0f},
        glm::vec3{0.0f, 0.0f, 0.0f},
        glm::vec3{0.0f, 0.0f, 90.0f},
        glm::vec3{0.0f, 180.0f, 0.0f}
    };
    bool gravity = false;
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
                break;
            case SDL_MOUSEBUTTONDOWN:
                mousePos.x = e.button.x;
                mousePos.y = WINDOW_HEIGHT - e.button.y;
                clicked = true;
                std::cout << "Click coords " << mousePos.x << " " << mousePos.y << "\n";
            }
        }

        // Camera
        
        float z = glm::sin((float)SDL_GetTicks() / 1000.0f) * 5.0f;
        float x = glm::cos((float)SDL_GetTicks() / 1000.0f) * 5.0f + 1.5f;
        float y = glm::cos((float)SDL_GetTicks() / 4000.0f) * 5.0f; 
        glm::vec3 cameraPos{x, 3.0f, z};
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);
        glm::vec3 origin{0.0f};
        glm::mat4 view = glm::lookAt(cameraPos + playerPos, playerPos, glm::vec3{0.0f, 1.0f, 0.0f});

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
            playerPos = glm::vec3{0.0f};
        
        playerPos = playerPos + moveDir * speed;
        if(gravity)
            playerPos = playerPos + glm::vec3{0.0f, -0.1f, 0.0f};

        std::vector<glm::mat4> models;
        for(int i = 0; i < slopesPos.size(); i++)
        {
            auto slopePos = slopesPos[i] * boxSize;
            auto angle = slopesRotation[i];

            // Collision player and slope i
            auto rotation = glm::rotate(glm::mat4{1.0f}, glm::radians(angle.y), glm::vec3{0.0f, 1.0f, 0.0f});
            rotation = glm::rotate(rotation, glm::radians(0.0f), glm::vec3{1.0f, 0.0f, 0.0f});
            rotation = glm::rotate(rotation, glm::radians(angle.z), glm::vec3{0.0f, 0.0f, 1.0f});
            
            glm::mat4 translation = glm::translate(glm::mat4{1.0f}, slopePos);
            auto transform = translation * rotation;
            auto posA = glm::inverse(transform) * glm::vec4{playerPos, 1.0f};

            auto boxIntersect = AABBSlopeCollision(posA, glm::vec3{1.0f}, boxSize);
            if(boxIntersect.intersects)
            {
                // Collision resolution in model space
                glm::vec3 offset = rotation * glm::vec4{boxIntersect.offset, 1.0f};
                auto normal = rotation * glm::vec4{boxIntersect.normal, 1.0f};

                // Check for slope orientation and fix offset is model space
                
                if (i & 1 && boxIntersect.normal.z == boxIntersect.normal.y)
                {
                    offset.z = 0.0f;
                    offset.y *= 2.0f;
                }

                auto isGround = normal.y > 0.0f;
                if(isGround)
                    grounded = true;
                
                PrintVec(offset, "World offset");
                PrintVec(normal, "World normal");
                playerPos = playerPos + offset;
            }

            // Calculate cube/slope model matrix
            glm::mat4 model{1.0f};
            model = glm::translate(model, slopePos);
            model = glm::scale(model, boxSize);
            model = model * rotation;
            models.push_back(model);
        }

        // Gravity
        std::cout << "IsGrounded: " << grounded << '\n';
        gravity = !grounded;

        // Player transfrom
        glm::mat4 playerModel = glm::translate(glm::mat4{1.0f}, playerPos);
        auto playerTransform = projection * view * playerModel;

        // Ray intersection
        if(clicked)
        {
            // Window to eye
            glm::vec3 windowPos{mousePos.x, mousePos.y, 100.0f};
            glm::vec4 nd{(windowPos.x * 2.0f) / (float)WINDOW_WIDTH - 1.0f, (windowPos.y * 2.0f) / (float) WINDOW_HEIGHT - 1.0f, (2 * windowPos.z - 100.0f - 0.1f) / (100.f - 0.1f), 1.0f};
            glm::vec4 clipPos = nd / 1.0f;
            glm::mat4 screenToWorld = glm::inverse(projection * view);
            glm::vec4 worldRayDir = screenToWorld * clipPos;
            std::cout << "World ray dir is " << worldRayDir.x << " " << worldRayDir.y << " " << worldRayDir.z << "\n";

            // Check intersection
            for(auto model : models)
            {
                glm::mat4 worldToModel = glm::inverse(model);
                glm::vec3 rayOrigin = glm::vec3{worldToModel * glm::vec4(cameraPos, 1.0f)};
                std::cout << "Ray World origin is " << cameraPos.x << " " << cameraPos.y << " " << cameraPos.z << "\n";
                std::cout << "Ray Model origin is " << rayOrigin.x << " " << rayOrigin.y << " " << rayOrigin.z << "\n";
                glm::vec3 rayDir = glm::normalize(glm::vec3{worldToModel * worldRayDir});

                auto intersection = RayAABBIntersection(rayOrigin, rayDir, boxSize);
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
            auto transform = projection * view * model;

            slopeVao.Bind();
            shader.SetUniformInt("isPlayer", 0);
            shader.SetUniformMat4("transform", transform);
            glDrawElements(GL_TRIANGLES, slopeVao.GetIndicesCount(), GL_UNSIGNED_INT, 0);
        }

        // Draw Player
        vao.Bind();
        shader.SetUniformInt("isPlayer", 1);
        shader.SetUniformMat4("transform", playerTransform);
        glDrawElements(GL_TRIANGLES, vao.GetIndicesCount(), GL_UNSIGNED_INT, 0);

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