#include <Editor.h>

#include <iostream>
#include <algorithm>

#include <glm/gtc/constants.hpp>

void BlockBuster::Editor::Start()
{
    // Shaders
    shader.Use();

    // Meshes
    cube = Rendering::Primitive::GenerateCube();
    slope = Rendering::Primitive::GenerateSlope();

    // Textures
    texture = GL::Texture::FromFolder(TEXTURES_DIR, "SmoothStone.png");
    try
    {
        texture.Load();
    }
    catch(const GL::Texture::LoadError& e)
    {
        std::cout << "Error when loading texture " + e.path_.string() + ": " +  e.what() << '\n';
    }
    
    // OpenGL features
    glEnable(GL_DEPTH_TEST);

    // Camera pos
    camera.SetPos(glm::vec3 {0.0f, 6.0f, 6.0f});
    camera.SetParam(Rendering::Camera::Param::ASPECT_RATIO, (float)config.window.width / (float)config.window.height);
    camera.SetTarget(glm::vec3{0.0f});

    // World
    blocks = {
        {Math::Transform{glm::vec3{0.0f, 0.0f, 0.0f} * BLOCK_SCALE, glm::vec3{0.0f, 0.0f, 0.0f}, BLOCK_SCALE}, Game::BLOCK},
        {Math::Transform{glm::vec3{0.0f, 6.0f, 0.0f} * BLOCK_SCALE, glm::vec3{0.0f, 0.0f, 0.0f}, BLOCK_SCALE}, Game::BLOCK},
    };
}

void BlockBuster::Editor::Update()
{
    // Handle Events    
    SDL_Event e;
    while(SDL_PollEvent(&e) != 0)
    {
        ImGui_ImplSDL2_ProcessEvent(&e);
        ImGuiIO& io = ImGui::GetIO();
        
        switch(e.type)
        {
        case SDL_KEYDOWN:
            if(e.key.keysym.sym == SDLK_ESCAPE)
                quit = true;
            break;
        case SDL_QUIT:
            std::cout << "Quitting\n";
            quit = true;
            break;
        case SDL_MOUSEBUTTONDOWN:
            if(!io.WantCaptureMouse)
            {
                glm::vec2 mousePos;
                mousePos.x = e.button.x;
                mousePos.y = config.window.height - e.button.y;
                UseTool(mousePos);
                std::cout << "Click coords " << mousePos.x << " " << mousePos.y << "\n";
            }
        }
    }
    // Camera
    UpdateCamera();

    // Draw cubes/slopes
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
            cube.Draw(shader, &texture);
        }
    }

    // GUI
    GUI();

    SDL_GL_SwapWindow(window_);
}

bool BlockBuster::Editor::Quit()
{
    return quit;
}

void BlockBuster::Editor::UpdateCamera()
{
    auto state = SDL_GetKeyboardState(nullptr);

    auto cameraRot = camera.GetRotation();
    float pitch = 0.0f;
    float yaw = 0.0f;
    if(state[SDL_SCANCODE_UP])
        pitch += -CAMERA_ROT_SPEED;
    if(state[SDL_SCANCODE_DOWN])
        pitch += CAMERA_ROT_SPEED;

    if(state[SDL_SCANCODE_LEFT])
        yaw += CAMERA_ROT_SPEED;
    if(state[SDL_SCANCODE_RIGHT])
        yaw += -CAMERA_ROT_SPEED;
    
    cameraRot.x = glm::max(glm::min(cameraRot.x + pitch, glm::pi<float>() - CAMERA_ROT_SPEED), CAMERA_ROT_SPEED);
    cameraRot.y = cameraRot.y + yaw;
    camera.SetRotation(cameraRot.x, cameraRot.y);
    
    auto cameraPos = camera.GetPos();
    auto front = camera.GetFront();
    auto xAxis = glm::normalize(-glm::cross(front, Rendering::Camera::UP));
    auto zAxis = glm::normalize(-glm::cross(xAxis, Rendering::Camera::UP));
    glm::vec3 moveDir{0};
    if(state[SDL_SCANCODE_A])
        moveDir += xAxis;
    if(state[SDL_SCANCODE_D])
        moveDir -= xAxis;
    if(state[SDL_SCANCODE_W])
        moveDir -= zAxis;
    if(state[SDL_SCANCODE_S])
        moveDir += zAxis;
    if(state[SDL_SCANCODE_Q])
        moveDir.y += 1;
    if(state[SDL_SCANCODE_E])
        moveDir.y -= 1;
    if(state[SDL_SCANCODE_F])
        cameraPos = glm::vec3{0.0f, 2.0f, 0.0f};
           
    auto offset = glm::length(moveDir) > 0.0f ? (glm::normalize(moveDir) * CAMERA_MOVE_SPEED) : moveDir;
    cameraPos += offset;
    camera.SetPos(cameraPos);
}

void BlockBuster::Editor::GUI()
{
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(window_);
    ImGui::NewFrame();

    bool open;
    ImGui::SetNextWindowPos(ImVec2{0, 0}, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2{(float)config.window.width, 0}, ImGuiCond_Always);
    if(ImGui::Begin("Editor", &open, ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
    {
        ImGui::Text("Tools");
        bool pbSelected = tool == PLACE_BLOCK;
        bool rbSelected = tool == REMOVE_BLOCK;

        if(ImGui::BeginTable("Tools", 2, 0, ImVec2{180, 0}))
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            if(ImGui::Selectable("Place Block", &pbSelected, 0, ImVec2{80, 20}))
            {
                std::cout << "Placing block enabled\n";
                tool = PLACE_BLOCK;
            }
            ImGui::TableNextColumn();
            if(ImGui::Selectable("Remove Block", &rbSelected, 0, ImVec2{120, 20}))
            {
                std::cout << "Removing block enabled\n";
                tool = REMOVE_BLOCK;
            }

            ImGui::EndTable();
        }

        ImGui::Text("Block");
        ImGui::ColorEdit4("ColorEdit", &color.x);
        ImGui::Text("Block Type");
        
        
        if(ImGui::RadioButton("Block", blockType == Game::BlockType::BLOCK))
        {
            blockType = Game::BlockType::BLOCK;
        }
        ImGui::SameLine();

        if(ImGui::RadioButton("Slope", blockType == Game::BlockType::SLOPE))
        {
            blockType = Game::BlockType::SLOPE;
        }
    }

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void BlockBuster::Editor::UseTool(glm::vec<2, int> mousePos)
{
    // Window to eye
    auto ray = Rendering::ScreenToWorldRay(camera, mousePos, glm::vec2{config.window.width, config.window.height});

    // Sort blocks by proximity to camera
    auto cameraPos = this->camera.GetPos();
    std::sort(blocks.begin(), blocks.end(), [cameraPos](Game::Block a, Game::Block b)
    {
        auto toA = glm::length(a.transform.position - cameraPos);
        auto toB = glm::length(b.transform.position- cameraPos);
        return toA < toB;
    });

    // Check intersection
    int index = -1;
    Collisions::RayIntersection intersection;
    for(int i = 0; i < blocks.size(); i++)
    {  
        auto model = blocks[i].transform.GetTransformMat();
        auto type = blocks[i].type;

        if(type == Game::SLOPE)
            intersection = Collisions::RaySlopeIntersection(ray, model);
        else
            intersection = Collisions::RayAABBIntersection(ray, model);

        if(intersection.intersects)
        {
            index = i;
            break;
        }
    }
    
    if(index != -1)
    {
        if(tool == PLACE_BLOCK)
        {
            auto block = blocks[index];
            auto pos = block.transform.position;
            auto angle = block.transform.rotation;
            auto scale = block.transform.scale;

            auto newBlockPos = pos + intersection.normal * scale;
            auto newBlockType = blockType;
            auto newBlockRot = glm::vec3{0.0f};

            blocks.push_back({Math::Transform{newBlockPos, newBlockRot, scale}, newBlockType});
        }
        else if(tool == REMOVE_BLOCK)
        {
            if(blocks.size() > 1)
                blocks.erase(blocks.begin() + index);
        }
    }
}