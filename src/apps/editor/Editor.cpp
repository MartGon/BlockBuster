#include <Editor.h>

#include <iostream>
#include <algorithm>
#include <fstream>
#include <cstring>

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
    int width, height;
    SDL_GetWindowSize(window_, &width, &height);
    camera.SetParam(Rendering::Camera::Param::ASPECT_RATIO, (float)width / (float)height);
    camera.SetTarget(glm::vec3{0.0f});

    // World
    InitMap();

    // Gui Setup
    RenameWindow("New Map");
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
            quit = true;
            break;
        case SDL_WINDOWEVENT:
            HandleWindowEvent(e.window);
            break;
        case SDL_MOUSEBUTTONDOWN:
            if(!io.WantCaptureMouse)
            {
                glm::vec2 mousePos;
                mousePos.x = e.button.x;
                mousePos.y = config.window.height - e.button.y;
                UseTool(mousePos, e.button.button == SDL_BUTTON_RIGHT);
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
        auto block = blocks[i];
        auto model = block.transform.GetTransformMat();
        auto type = block.type;
        auto transform = camera.GetProjViewMat() * model;

        shader.SetUniformInt("isPlayer", 0);
        shader.SetUniformMat4("transform", transform);
        auto& mesh = GetMesh(block.type);
        auto display = block.display;
        if(display.type == Game::DisplayType::TEXTURE)
            mesh.Draw(shader, &texture);
        else if(display.type == Game::DisplayType::COLOR)
            mesh.Draw(shader, display.display.color.color);
    }

    // GUI
    GUI();

    SDL_GL_SwapWindow(window_);
}

bool BlockBuster::Editor::Quit()
{
    return quit;
}

Rendering::Mesh& BlockBuster::Editor::GetMesh(Game::BlockType blockType)
{
    return blockType == Game::BlockType::SLOPE ? slope : cube;
}

Game::Block* BlockBuster::Editor::GetBlock(glm::vec3 pos)
{   
    Game::Block* block = nullptr;
    for(auto& b : blocks)
        if(b.transform.position == pos)
            block = &b;

    return block;
}

template<typename T>
void WriteToFile(std::fstream& file, T val)
{
    file.write(reinterpret_cast<char*>(&val), sizeof(T));
}

template <typename T>
T ReadFromFile(std::fstream& file)
{
    T val;
    file.read(reinterpret_cast<char*>(&val), sizeof(T));
    return val;
}

void SaveVec2(std::fstream& file, glm::vec2 vec)
{
    WriteToFile(file, vec.x);
    WriteToFile(file, vec.y);
}

glm::vec2 ReadVec2(std::fstream& file)
{
    glm::vec2 vec;
    vec.x = ReadFromFile<float>(file);
    vec.y = ReadFromFile<float>(file);

    return vec;
}

void SaveVec3(std::fstream& file, glm::vec3 vec)
{
    SaveVec2(file, vec);
    WriteToFile(file, vec.z);
}

glm::vec3 ReadVec3(std::fstream& file)
{
    auto vec2 = ReadVec2(file);
    float z = ReadFromFile<float>(file);

    return glm::vec3{vec2, z};
}

void SaveVec4(std::fstream& file, glm::vec4 vec)
{
    SaveVec3(file, vec);
    WriteToFile(file, vec.w);
}

glm::vec4 ReadVec4(std::fstream& file)
{
    glm::vec3 vec3 = ReadVec3(file);
    float w = ReadFromFile<float>(file);
    return glm::vec4{vec3, w};
}

void BlockBuster::Editor::InitMap()
{
    blocks = {
        {Math::Transform{glm::vec3{0.0f, 0.0f, 0.0f} * blockScale, glm::vec3{0.0f, 0.0f, 0.0f}, blockScale}, Game::BLOCK},
        //{Math::Transform{glm::vec3{0.0f, 6.0f, 0.0f} * BLOCK_SCALE, glm::vec3{0.0f, 0.0f, 0.0f}, BLOCK_SCALE}, Game::BLOCK},
    };
}

void BlockBuster::Editor::SaveMap()
{
    std::fstream file{fileName, file.binary | file.out};
    if(!file.is_open())
    {
        std::cout << "Could not open file " << fileName << '\n';
        return;
    }

    // Header
    WriteToFile(file, magicNumber);
    WriteToFile(file, blocks.size());
    WriteToFile(file, blockScale);

    // Camera Pos/Rot
    SaveVec3(file, camera.GetPos());
    SaveVec2(file, camera.GetRotation());

    // Blocks
    for(auto& block : blocks)
    {
        WriteToFile(file, block.type);
        SaveVec3(file, block.transform.position);
        if(block.type == Game::BlockType::SLOPE)
            SaveVec3(file, block.transform.rotation);

        WriteToFile(file, block.display.type);
        if(block.display.type == Game::DisplayType::COLOR)
            SaveVec4(file, block.display.display.color.color);
        else
            WriteToFile(file, block.display.display.texture.textureId);
    }
}

bool BlockBuster::Editor::LoadMap()
{
    std::fstream file{fileName, file.binary | file.in};
    if(!file.is_open())
    {
        std::cout << "Could not open file " << fileName << '\n';
        return false;
    }

    auto magic = ReadFromFile<int>(file);
    if(magic != magicNumber)
    {
        std::cout << "Wrong file format for file " << fileName << "\n";
        return false;
    }

    auto count = ReadFromFile<std::size_t>(file);
    blockScale = ReadFromFile<float>(file);

    auto cameraPos = ReadVec3(file);
    auto cameraRot = ReadVec2(file);
    camera.SetPos(cameraPos);
    camera.SetRotation(cameraRot.x, cameraRot.y);

    blocks.clear();
    blocks.reserve(count);
    for(std::size_t i = 0; i < count; i++)
    {
        Game::Block block;
        block.type = ReadFromFile<Game::BlockType>(file);
        block.transform.position = ReadVec3(file);
        block.transform.scale = blockScale;
        if(block.type == Game::BlockType::SLOPE)
            block.transform.rotation = ReadVec3(file);

        block.display.type = ReadFromFile<Game::DisplayType>(file);
        if(block.display.type == Game::DisplayType::COLOR)
            block.display.display.color.color = ReadVec4(file);
        else
            block.display.display.texture.textureId = ReadFromFile<int>(file);

        blocks.push_back(block);
    }

    return true;
}

void BlockBuster::Editor::UpdateCamera()
{
    if(io_->WantCaptureKeyboard)
        return;

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

void BlockBuster::Editor::UseTool(glm::vec<2, int> mousePos, bool rightButton)
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
        // Use appropiate Tool
        switch(tool)
        {
            case PLACE_BLOCK:
            {
                if(rightButton)
                {
                    if(blocks.size() > 1)
                        blocks.erase(blocks.begin() + index);
                    break;
                }
                else
                {
                    auto yaw = (std::round(glm::degrees(camera.GetRotation().y) / 90.0f) * 90.0f) - 90.0f;

                    auto block = blocks[index];
                    auto pos = block.transform.position;
                    auto scale = block.transform.scale;

                    auto newBlockPos = pos + intersection.normal * scale;
                    auto newBlockRot = glm::vec3{0.0f, yaw, 0.0f};
                    auto newBlockType = blockType;

                    Game::Display display;
                    display.type = displayType;
                    if(displayType == Game::DisplayType::COLOR)
                        display.display.color.color = displayColor;
                    else if(displayType == Game::DisplayType::TEXTURE)
                        display.display.texture.textureId = 0;

                    if(!GetBlock(newBlockPos))
                    {
                        blocks.push_back({Math::Transform{newBlockPos, newBlockRot, scale}, newBlockType, display});
                        std::cout << "Block added at \n";
                    }
                }

                break;
            }            
            case ROTATE_BLOCK:
            {
                auto& block = blocks[index];
                float mod = rightButton ? -90.0f : 90.0f;
                if(block.type == Game::BlockType::SLOPE)
                {
                    if(axis == RotationAxis::X)
                        block.transform.rotation.x += mod;
                    else if(axis == RotationAxis::Y)
                        block.transform.rotation.y += mod;
                    else
                        block.transform.rotation.z += mod;
                    break;
                }
            }
        }
    }
}

void BlockBuster::Editor::RenameWindow(std::string name)
{
    std::string title = "Editor - " + name;
    SDL_SetWindowTitle(window_, title.c_str());
}

void BlockBuster::Editor::SaveAsPopUp()
{
    if(state == PopUpState::SAVE_AS)
        ImGui::OpenPopup("Save as");

    auto displaySize = io_->DisplaySize;
    ImGui::SetNextWindowPos(ImVec2{displaySize.x * 0.5f, displaySize.y * 0.5f}, ImGuiCond_Always, ImVec2{0.5f, 0.5f});
    if(ImGui::BeginPopupModal("Save as", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
    {   
        ImGui::InputText("File Name", fileName, 16, ImGuiInputTextFlags_EnterReturnsTrue);
        if(ImGui::Button("Accept"))
        {
            SaveMap();
            RenameWindow(fileName);
            newMap = false;

            ImGui::CloseCurrentPopup();
            state = PopUpState::NONE;
        }

        ImGui::SameLine();
        if(ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
            state = PopUpState::NONE;
        }

        ImGui::EndPopup();
    }
}

void BlockBuster::Editor::OpenMapPopUp()
{
    if(state == PopUpState::OPEN_MAP)
        ImGui::OpenPopup("Open Map");

    auto displaySize = io_->DisplaySize;
    ImGui::SetNextWindowPos(ImVec2{displaySize.x * 0.5f, displaySize.y * 0.5f}, ImGuiCond_Always, ImVec2{0.5f, 0.5f});
    if(ImGui::BeginPopupModal("Open Map", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
    {   
        ImGui::InputText("File Name", fileName, 16, ImGuiInputTextFlags_EnterReturnsTrue);
        if(ImGui::Button("Accept"))
        {
            if(LoadMap())
            {
                RenameWindow(fileName);
                newMap = false;
                errorText = "";

                ImGui::CloseCurrentPopup();
                state = PopUpState::NONE;
            }
            else
                errorText = "Could not load map " + std::string(fileName);
        }

        ImGui::SameLine();
        if(ImGui::Button("Cancel"))
        {
            errorText = "";
            ImGui::CloseCurrentPopup();
            state = PopUpState::NONE;
        }

        if(!errorText.empty())
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", errorText.c_str());

        ImGui::EndPopup();
    }
}

std::vector<SDL_DisplayMode> GetDisplayModes()
{
    std::vector<SDL_DisplayMode> displayModes;

    int displays = SDL_GetNumVideoDisplays();
    for(int i = 0; i < 1; i++)
    {
        auto numDisplayModes = SDL_GetNumDisplayModes(i);

        displayModes.reserve(numDisplayModes);
        for(int j = 0; j < numDisplayModes; j++)
        {
            SDL_DisplayMode mode;
            if(!SDL_GetDisplayMode(i, j, &mode))
            {
                displayModes.push_back(mode);
            }
        }
    }

    return displayModes;
}

void BlockBuster::Editor::HandleWindowEvent(SDL_WindowEvent winEvent)
{
    std::cout << "Window event type " << (int)winEvent.event << "\n";

    if(winEvent.event == SDL_WINDOWEVENT_RESIZED)
    {
        int width = winEvent.data1;
        int height = winEvent.data2;
        std::cout << "Resized/Restored to " << width << " " << height <<"\n";
        auto flags = SDL_GetWindowFlags(window_);
        glViewport(0, 0, width, height);
        camera.SetParam(camera.ASPECT_RATIO, (float) width / (float) height);
    }
}

void BlockBuster::Editor::ApplyVideoOptions(::App::Configuration& config)
{
    auto& winConfig = config.window;
    auto width = winConfig.width;
    auto height = winConfig.height;
    SDL_SetWindowFullscreen(window_, winConfig.mode);

    if(winConfig.mode == ::App::Configuration::FULLSCREEN)
    {
        SDL_SetWindowDisplayMode(window_, NULL);
        auto display = SDL_GetWindowDisplayIndex(window_);
        SDL_DisplayMode mode;
        SDL_GetDesktopDisplayMode(display, &mode);
        width = mode.w;
        height = mode.h;
    }
    SDL_SetWindowSize(window_, width, height);

    SDL_GetWindowSize(window_, &width, &height);
    glViewport(0, 0, width, height);
    camera.SetParam(camera.ASPECT_RATIO, (float) width / (float) height);
    SDL_GL_SetSwapInterval(winConfig.vsync);

    SDL_SetWindowPosition(window_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

std::string DisplayModeToString(int w, int h, int rr)
{
    return std::to_string(w) + " x " + std::to_string(h) + " " + std::to_string(rr) + " Hz";
}

std::string DisplayModeToString(SDL_DisplayMode mode)
{
    return std::to_string(mode.w) + " x " + std::to_string(mode.h) + " " + std::to_string(mode.refresh_rate) + " Hz";
}

void BlockBuster::Editor::VideoOptionsPopUp()
{
    if(state == PopUpState::VIDEO_SETTINGS)
        ImGui::OpenPopup("Video");

    auto displaySize = io_->DisplaySize;
    ImGui::SetNextWindowPos(ImVec2{displaySize.x * 0.5f, displaySize.y * 0.5f}, ImGuiCond_Always, ImVec2{0.5f, 0.5f});

    bool closed = onPopUp;
    if(ImGui::BeginPopupModal("Video", &onPopUp, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
    {   
        std::string resolution = DisplayModeToString(preConfig.window.width, preConfig.window.height, preConfig.window.refreshRate);
        if(ImGui::BeginCombo("Resolution", resolution.c_str()))
        {
            auto displayModes = GetDisplayModes();
            for(auto& mode : displayModes)
            {
                bool selected = preConfig.window.mode == mode.w && preConfig.window.mode == mode.h && preConfig.window.refreshRate == mode.refresh_rate;
                if(ImGui::Selectable(DisplayModeToString(mode).c_str(), selected))
                {
                    preConfig.window.width = mode.w;
                    preConfig.window.height = mode.h;
                    preConfig.window.refreshRate = mode.refresh_rate;
                }
            }

            ImGui::EndCombo();
        }

        ImGui::Text("Window Mode");
        ImGui::RadioButton("Windowed", &preConfig.window.mode, ::App::Configuration::WindowMode::WINDOW); ImGui::SameLine();
        ImGui::RadioButton("Fullscreen", &preConfig.window.mode, ::App::Configuration::WindowMode::FULLSCREEN); ImGui::SameLine();
        ImGui::RadioButton("Borderless", &preConfig.window.mode, ::App::Configuration::WindowMode::BORDERLESS);

        ImGui::Checkbox("Vsync", &preConfig.window.vsync);

        if(ImGui::Button("Accept"))
        {
            ApplyVideoOptions(preConfig);
            config = preConfig;
            state = PopUpState::NONE;

            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if(ImGui::Button("Apply"))
        {
            ApplyVideoOptions(preConfig);
        }

        ImGui::SameLine();
        if(ImGui::Button("Cancel"))
        {
            ApplyVideoOptions(config);
            preConfig = config;
            state = PopUpState::NONE;
            
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
    else if(state == PopUpState::VIDEO_SETTINGS)
    {
        std::cout << "Window was closed\n";
        state = PopUpState::NONE;
        ApplyVideoOptions(config);
        preConfig = config;
    }
}

void BlockBuster::Editor::MenuBar()
{
    // Pop Ups
    OpenMapPopUp();
    SaveAsPopUp();
    VideoOptionsPopUp();

    if(ImGui::BeginMenuBar())
    {
        if(ImGui::BeginMenu("File", true))
        {
            if(ImGui::MenuItem("New Map", "Ctrl + N"))
            {
                InitMap();
                std::strcpy(fileName, "NewMap.bbm");
                RenameWindow("New Map");
                newMap = true;
            }

            ImGui::Separator();

            if(ImGui::MenuItem("Open Map", "Ctrl + O"))
            {
                state = PopUpState::OPEN_MAP;
            }

            ImGui::Separator();

            if(ImGui::MenuItem("Save", "Ctrl + S"))
            {
                if(newMap)
                    state = PopUpState::SAVE_AS;
                else
                    SaveMap();
            }

            if(ImGui::MenuItem("Save As", "Ctrl + Shift + S"))
            {
                state = PopUpState::SAVE_AS;
            }

            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("Edit", true))
        {
            if(ImGui::MenuItem("Undo", "Ctrl + Z"))
            {
                std::cout << "Undoing\n";
            }

            if(ImGui::MenuItem("Redo", "Ctrl + Shift + Z"))
            {
                std::cout << "Redoing\n";
            }

            ImGui::Separator();

            if(ImGui::MenuItem("Go to Block", "Ctrl + G"))
            {
                std::cout << "Going to block\n";
            }

            ImGui::Separator();

            if(ImGui::MenuItem("Mouse camera mode", "C", mouseCamera))
            {
                mouseCamera = !mouseCamera;
            }

            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("Settings", true))
        {
            if(ImGui::MenuItem("Video", "Ctrl + Shift + G"))
            {
                state = PopUpState::VIDEO_SETTINGS;
                preConfig = config;
                onPopUp = true;
            }

            if(ImGui::MenuItem("Language"))
            {

            }

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void BlockBuster::Editor::GUI()
{
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(window_);
    ImGui::NewFrame();

    bool open;
    auto displaySize = io_->DisplaySize;
    ImGui::SetNextWindowPos(ImVec2{0, 0}, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2{(float)displaySize.x, 0}, ImGuiCond_Always);
    auto windowFlags = ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_MenuBar;
    if(ImGui::Begin("Editor", &open, windowFlags))
    {
        MenuBar();

        bool pbSelected = tool == PLACE_BLOCK;
        bool rotbSelected = tool == ROTATE_BLOCK;

        ImGuiTableFlags tableFlags = ImGuiTableFlags_SizingFixedFit;
        ImGui::SetCursorPosX(0);
        if(ImGui::BeginTable("#Tools", 2, 0, ImVec2{0, 0}))
        {
            // Title Column 1
            ImGui::TableSetupColumn("##Tools", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("##Tool Options", 0);
            
            ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
            ImGui::TableSetColumnIndex(0);
            ImGui::TableHeader("##Tools");
            ImGui::SameLine();
            ImGui::Text("Tools");

            ImGui::TableSetColumnIndex(1);
            ImGui::TableHeader("##Tool Options");
            ImGui::SameLine();
            ImGui::Text("Tools Options");
            
            // Tools Table
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            if(ImGui::BeginTable("##Tools", 2, 0, ImVec2{120, 0}))
            {
                ImGui::TableNextColumn();
                ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_SelectableTextAlign, {0.5, 0});
                if(ImGui::Selectable("Place", &pbSelected, 0, ImVec2{0, 0}))
                {
                    std::cout << "Placing block enabled\n";
                    tool = PLACE_BLOCK;
                }

                ImGui::TableNextColumn();
                if(ImGui::Selectable("Rotate", &rotbSelected, 0, ImVec2{0, 0}))
                {
                    std::cout << "Rotating block enabled\n";
                    tool = ROTATE_BLOCK;
                }
                ImGui::PopStyleVar();

                ImGui::EndTable();
            }

            // Tools Options
            ImGui::TableNextColumn();

            if(pbSelected)
            {
                ImGui::Text("Block Type");
                ImGui::SameLine();
                ImGui::RadioButton("Block", &blockType, Game::BlockType::BLOCK);
                ImGui::SameLine();

                ImGui::RadioButton("Slope", &blockType, Game::BlockType::SLOPE);

                ImGui::Text("Display Type");
                ImGui::SameLine();
                ImGui::RadioButton("Texture", &displayType, Game::DisplayType::TEXTURE);
                ImGui::SameLine();

                ImGui::RadioButton("Color", &displayType, Game::DisplayType::COLOR);

                if(displayType == Game::DisplayType::COLOR)
                {
                    ImGui::Text("Color");
                    ImGui::SameLine();
                    ImGui::ColorEdit4("", &displayColor.x);
                }
            }

            if(rotbSelected)            
            {
                ImGui::Text("Axis");

                ImGui::SameLine();
                ImGui::RadioButton("X", &axis, RotationAxis::X);
                ImGui::SameLine();
                ImGui::RadioButton("Y", &axis, RotationAxis::Y);
                ImGui::SameLine();
                ImGui::RadioButton("Z", &axis, RotationAxis::Z);
            }

            ImGui::EndTable();
        }
    }

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}