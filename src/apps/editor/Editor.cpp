#include <Editor.h>

#include <iostream>
#include <algorithm>
#include <fstream>
#include <cstring>

#include <glm/gtc/constants.hpp>

// #### Public Interface #### \\

void BlockBuster::Editor::Start()
{
    // Shaders
    shader.Use();

    // Meshes
    cube = Rendering::Primitive::GenerateCube();
    slope = Rendering::Primitive::GenerateSlope();

    // Textures
    textureFolder = GetConfigOption("TextureFolder", TEXTURES_DIR);
    GL::Texture texture = GL::Texture::FromFolder(textureFolder, "SmoothStone.png");
    try
    {
        texture.Load();
    }
    catch(const GL::Texture::LoadError& e)
    {
        std::cout << "Error when loading texture " + e.path_.string() + ": " +  e.what() << '\n';
    }
    textures.reserve(MAX_TEXTURES);
    textures.push_back(std::move(texture));
    
    // OpenGL features
    glEnable(GL_DEPTH_TEST);

    // Camera pos
    int width, height;
    SDL_GetWindowSize(window_, &width, &height);
    camera.SetParam(Rendering::Camera::Param::ASPECT_RATIO, (float)width / (float)height);
    
    // World
    auto mapName = GetConfigOption("Map", "Map.bbm");
    if(!mapName.empty() && mapName.size() < 16)
    {
        std::strcpy(fileName, mapName.c_str());
        LoadMap();
    }
    else
        NewMap();
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
                mousePos.y = GetWindowSize().y - e.button.y;
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
            mesh.Draw(shader, &textures[display.id]);
        else if(display.type == Game::DisplayType::COLOR)
            mesh.Draw(shader, colors[display.id]);
    }

    // GUI
    GUI();

    SDL_GL_SwapWindow(window_);
}

bool BlockBuster::Editor::Quit()
{
    return quit;
}

void BlockBuster::Editor::Shutdown()
{
    config.options["Map"] = std::string(fileName);
    config.options["TextureFolder"] = textureFolder.string();
}

// #### Rendering #### \\

Rendering::Mesh& BlockBuster::Editor::GetMesh(Game::BlockType blockType)
{
    return blockType == Game::BlockType::SLOPE ? slope : cube;
}

bool BlockBuster::Editor::LoadTexture()
{
    if(textures.size() >= MAX_TEXTURES)
        return false;

    auto texture = GL::Texture::FromFolder(textureFolder, textureFilename);
    try
    {
        texture.Load();
    }
    catch(const GL::Texture::LoadError& e)
    {
        return false;
    }
    textures.push_back(std::move(texture));

    return true;
}

// #### World #### \\

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

Game::Block* BlockBuster::Editor::GetBlock(glm::vec3 pos)
{   
    Game::Block* block = nullptr;
    for(auto& b : blocks)
        if(b.transform.position == pos)
            block = &b;

    return block;
}

void BlockBuster::Editor::NewMap()
{
    // Texture pallete
    textures.clear();

    // Color palette
    colors = {glm::vec4{0.0f, 0.0f, 0.0f, 1.0f}, glm::vec4{1.0f, 0.0f, 0.0f, 1.0f}, glm::vec4{1.0f}};

    // Camera
    camera.SetPos(glm::vec3 {0.0f, 6.0f, 6.0f});
    camera.SetTarget(glm::vec3{0.0f});

    // Set first block
    blocks = {
        {
            Math::Transform{glm::vec3{0.0f, 0.0f, 0.0f} * blockScale, glm::vec3{0.0f, 0.0f, 0.0f}, blockScale}, 
            Game::BLOCK, Game::Display{Game::DisplayType::COLOR, 2}
        },
    };

    // Window
    RenameMainWindow("New Map");
    newMap = true;

    // Filename
    std::strcpy(fileName, "NewMap.bbm");
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

    // Texture Table
    WriteToFile(file, textures.size());
    for(auto i = 0; i < textures.size(); i++)
    {
        auto texturePath =  textures[i].GetPath().string();
        for(auto c : texturePath)
            WriteToFile(file, c);
        WriteToFile(file, '\0');
    }

    // Colors table
    WriteToFile(file, colors.size());
    for(auto i = 0; i < colors.size(); i++)
    {
        SaveVec4(file, colors[i]);
    }

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
        {
            auto colorId = block.display.id;
            WriteToFile(file, colorId);
        }
        else
            WriteToFile(file, block.display.id);
    }

    // Update flag
    newMap = false;
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

    // Texture Table
    auto textureSize = ReadFromFile<std::size_t>(file);
    textures.clear();
    textures.reserve(textureSize);
    for(auto i = 0; i < textureSize; i++)
    {
        std::string texturePath;
        char c = ReadFromFile<char>(file);
        while(c != '\0' && !file.eof())
        {   
            texturePath.push_back(c);
            c = ReadFromFile<char>(file);
        }

        GL::Texture texture{texturePath};
        try{
            texture.Load();
        }
        catch(const GL::Texture::LoadError& e)
        {
            std::cout << "Could not load texture file " << texturePath << "\n";
        }
        textures.push_back(std::move(texture));
    }

    // Color Table
    auto colorsSize = ReadFromFile<std::size_t>(file);
    colors.clear();
    colors.reserve(colorsSize);
    for(auto i = 0; i < colorsSize; i++)
    {
        auto color = ReadVec4(file);
        colors.push_back(color);
    }

    // Camera Pos/Rot
    auto cameraPos = ReadVec3(file);
    auto cameraRot = ReadVec2(file);
    camera.SetPos(cameraPos);
    camera.SetRotation(cameraRot.x, cameraRot.y);

    // Blocks
    blocks.clear();
    blocks.reserve(count);
    for(auto i = 0; i < count; i++)
    {
        Game::Block block;
        block.type = ReadFromFile<Game::BlockType>(file);
        block.transform.position = ReadVec3(file);
        block.transform.scale = blockScale;
        if(block.type == Game::BlockType::SLOPE)
            block.transform.rotation = ReadVec3(file);

        block.display.type = ReadFromFile<Game::DisplayType>(file);
        if(block.display.type == Game::DisplayType::COLOR)
        {
            auto colorId = ReadFromFile<int>(file);
            block.display.id = colorId;
        }
        else
            block.display.id = ReadFromFile<int>(file);

        blocks.push_back(block);
    }

    // Window
    RenameMainWindow(fileName);
    newMap = false;

    // Color Palette
    colorPick = colors[colorId];

    return true;
}

// #### Editor #### \\

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
    auto winSize = GetWindowSize();
    auto ray = Rendering::ScreenToWorldRay(camera, mousePos, glm::vec2{winSize.x, winSize.y});

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
                    auto yaw = blockType == Game::BlockType::SLOPE ?  (std::round(glm::degrees(camera.GetRotation().y) / 90.0f) * 90.0f) - 90.0f : 0.0f;

                    auto block = blocks[index];
                    auto pos = block.transform.position;
                    auto scale = block.transform.scale;

                    auto newBlockPos = pos + intersection.normal * scale;
                    if(!GetBlock(newBlockPos))
                    {
                        auto display = GetBlockDisplay();
                        auto newBlockRot = glm::vec3{0.0f, yaw, 0.0f};
                        auto newBlockType = blockType;
                        blocks.push_back({Math::Transform{newBlockPos, newBlockRot, scale}, newBlockType, display});
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
            case PAINT_BLOCK:
            {
                auto& block = blocks[index];
                if(!rightButton)
                {
                    block.display = GetBlockDisplay();
                }
                else
                {
                    SetBlockDisplay(block.display);
                }
            }
        }
    }
}

Game::Display BlockBuster::Editor::GetBlockDisplay()
{
    Game::Display display;
    display.type = displayType;
    if(displayType == Game::DisplayType::COLOR)
        display.id = colorId;
    else if(displayType == Game::DisplayType::TEXTURE)
        display.id = textureId;

    return display;
}

void BlockBuster::Editor::SetBlockDisplay(Game::Display display)
{
    displayType = display.type;
    if(displayType == Game::DisplayType::TEXTURE)
        textureId = display.id;
    else if(displayType == Game::DisplayType::COLOR)
        colorId = display.id;
}

// #### Options #### \\

void BlockBuster::Editor::HandleWindowEvent(SDL_WindowEvent winEvent)
{
    if(winEvent.event == SDL_WINDOWEVENT_RESIZED)
    {
        int width = winEvent.data1;
        int height = winEvent.data2;
        auto flags = SDL_GetWindowFlags(window_);
        glViewport(0, 0, width, height);
        camera.SetParam(camera.ASPECT_RATIO, (float) width / (float) height);
    }
}

void BlockBuster::Editor::ApplyVideoOptions(::App::Configuration::WindowConfig& winConfig)
{
    auto width = winConfig.resolutionW;
    auto height = winConfig.resolutionH;
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

std::string BlockBuster::Editor::GetConfigOption(const std::string& key, std::string defaultValue)
{
    std::string ret = defaultValue;
    auto it = config.options.find(key);

    if(it != config.options.end())
    {
        ret = it->second;
    }

    return ret;
}

// #### GUI #### \\

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

void BlockBuster::Editor::LoadTexturePopUp()
{
    if(state == PopUpState::LOAD_TEXTURE)
        ImGui::OpenPopup("Load Texture");

    auto displaySize = io_->DisplaySize;
    ImGui::SetNextWindowPos(ImVec2{displaySize.x * 0.5f, displaySize.y * 0.5f}, ImGuiCond_Always, ImVec2{0.5f, 0.5f});
    if(ImGui::BeginPopupModal("Load Texture", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
    {   
        bool accept = ImGui::InputText("File Name", textureFilename, 32, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
        if(ImGui::Button("Accept") || accept)
        {
            if(LoadTexture())
            {
                ImGui::CloseCurrentPopup();
                state = PopUpState::NONE;
            }
            else
                errorText = "Could not open texture " + std::string(textureFilename);
        }

        ImGui::SameLine();
        if(ImGui::Button("Cancel"))
        {
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

    bool onPopUp = state == PopUpState::VIDEO_SETTINGS;
    if(ImGui::BeginPopupModal("Video", &onPopUp, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
    {   
        std::string resolution = DisplayModeToString(preConfig.resolutionW, preConfig.resolutionH, preConfig.refreshRate);
        if(ImGui::BeginCombo("Resolution", resolution.c_str()))
        {
            auto displayModes = GetDisplayModes();
            for(auto& mode : displayModes)
            {
                bool selected = preConfig.mode == mode.w && preConfig.mode == mode.h && preConfig.refreshRate == mode.refresh_rate;
                if(ImGui::Selectable(DisplayModeToString(mode).c_str(), selected))
                {
                    preConfig.resolutionW = mode.w;
                    preConfig.resolutionH = mode.h;
                    preConfig.refreshRate = mode.refresh_rate;
                }
            }

            ImGui::EndCombo();
        }

        ImGui::Text("Window Mode");
        ImGui::RadioButton("Windowed", &preConfig.mode, ::App::Configuration::WindowMode::WINDOW); ImGui::SameLine();
        ImGui::RadioButton("Fullscreen", &preConfig.mode, ::App::Configuration::WindowMode::FULLSCREEN); ImGui::SameLine();
        ImGui::RadioButton("Borderless", &preConfig.mode, ::App::Configuration::WindowMode::BORDERLESS);

        ImGui::Checkbox("Vsync", &preConfig.vsync);

        if(ImGui::Button("Accept"))
        {
            ApplyVideoOptions(preConfig);
            config.window = preConfig;
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
            ApplyVideoOptions(config.window);
            preConfig = config.window;
            state = PopUpState::NONE;
            
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
    // Triggered when X button is pressed, same effect as cancel
    else if(state == PopUpState::VIDEO_SETTINGS)
    {
        ApplyVideoOptions(config.window);
        preConfig = config.window;
        state = PopUpState::NONE;
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
                NewMap();
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
                preConfig = config.window;
            }

            if(ImGui::MenuItem("Language"))
            {

            }

            ImGui::Checkbox("Show demo window", &showDemo);

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
        bool paintSelected = tool == PAINT_BLOCK;

        ImGui::SetCursorPosX(0);
        auto tableFlags = ImGuiTableFlags_::ImGuiTableFlags_SizingFixedFit;
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

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if(ImGui::Selectable("Paint", &paintSelected, 0, ImVec2{0, 0}))
                {
                    std::cout << "Paint block enabled\n";
                    tool = PAINT_BLOCK;
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
            }

            if(pbSelected || paintSelected)
            {
                ImGui::Text("Display Type");
                ImGui::SameLine();
                ImGui::RadioButton("Texture", &displayType, Game::DisplayType::TEXTURE);
                ImGui::SameLine();

                ImGui::RadioButton("Color", &displayType, Game::DisplayType::COLOR);

                if(displayType == Game::DisplayType::COLOR)
                {
                    
                }
                else if(displayType == Game::DisplayType::TEXTURE)
                {
                    LoadTexturePopUp();
                }

                ImGui::Text("Palette");

                // Palette
                const glm::vec2 iconSize = displayType == Game::DisplayType::TEXTURE ? glm::vec2{32.f} : glm::vec2{20.f};
                const glm::vec2 selectSize = iconSize + glm::vec2{2.0f};
                const auto effectiveSize = glm::vec2{selectSize.x + 8.0f, selectSize.y + 6.0f};
                const auto MAX_ROWS = 2;
                const auto MAX_COLUMNS = 64;
                const auto scrollbarOffsetX = 14.0f;

                glm::vec2 region = ImGui::GetContentRegionAvail();
                int columns = glm::min((int)(region.x / effectiveSize.x), MAX_COLUMNS);
                int entries = displayType == Game::DisplayType::TEXTURE ? textures.size() : colors.size();
                int minRows = glm::max((int)glm::ceil((float)entries / (float)columns), 1);
                int rows = glm::min(MAX_ROWS, minRows);
                glm::vec2 tableSize = ImVec2{effectiveSize.x * columns + scrollbarOffsetX, effectiveSize.y * rows};
                auto tableFlags = ImGuiTableFlags_::ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY;
                if(ImGui::BeginTable("Texture Palette", columns, tableFlags, tableSize))
                {                        
                    for(unsigned int i = 0; i < entries; i++)
                    {
                        ImGui::TableNextColumn();
                        std::string label = "##selectable" + std::to_string(i);
                    
                        bool firstRowElement = (i % columns) == 0;
                        glm::vec2 offset = (firstRowElement ? glm::vec2{4.0f, 0.0f} : glm::vec2{0.0f});
                        glm::vec2 size = selectSize + offset;
                        glm::vec2 cursorPos = ImGui::GetCursorPos();
                        glm::vec2 pos = cursorPos + (size - iconSize) / 2.0f + offset / 2.0f;
                        
                        if(displayType == Game::DisplayType::COLOR)
                        {
                            auto newPos = cursorPos + glm::vec2{0.0f, -3.0f};
                            size += glm::vec2{-2.0f, 0.0f};
                            if(!firstRowElement)
                                ImGui::SetCursorPos(newPos);
                            
                            pos += glm::vec2{-1.0f, 0.0f};
                        }
                        
                        bool selected = displayType == Game::DisplayType::TEXTURE && i == textureId || 
                            displayType == Game::DisplayType::COLOR && i == colorId;
                        if(ImGui::Selectable(label.c_str(), selected, 0, size))
                        {
                            if(displayType == Game::DisplayType::TEXTURE)
                            {
                                textureId = i;
                            }
                            else if(displayType == Game::DisplayType::COLOR)
                            {
                                colorId = i;
                            }
                        }
                        ImGui::SetCursorPos(pos);
                        
                        if(displayType == Game::DisplayType::TEXTURE)
                        {
                            auto texture = &textures[i];
                            void* data = reinterpret_cast<void*>(texture->GetGLId());
                            ImGui::Image(data, iconSize);
                        }
                        else if(displayType == Game::DisplayType::COLOR)
                        {
                            ImGui::ColorButton("## color", colors[i]);
                        }
                    }       
                    ImGui::TableNextRow();
                    
                    ImGui::EndTable();
                }

                if(displayType == Game::DisplayType::COLOR)
                {
                    if(ImGui::ColorButton("Chosen Color", colors[colorId]))
                    {
                        ImGui::OpenPopup("Color Picker");
                        pickingColor = true;
                    }
                    if(ImGui::BeginPopup("Color Picker"))
                    {
                        if(ImGui::ColorPicker4("##picker", &colorPick.x, ImGuiColorEditFlags__OptionsDefault))
                            colorId = -1;
                        ImGui::EndPopup();
                    }
                    else if(pickingColor)
                    {
                        pickingColor = false;
                        if(displayType == Game::DisplayType::COLOR)
                        {
                            auto color = colorPick;
                            if(std::find(colors.begin(), colors.end(), color) == colors.end())
                            {
                                colors.push_back(colorPick);
                                colorId = colors.size() - 1;
                            }
                        }
                    }
                }
                else if(displayType == Game::DisplayType::TEXTURE)
                {
                    if(ImGui::Button("Add Texture"))
                    {
                        state = PopUpState::LOAD_TEXTURE;
                    }
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

    if(showDemo)
        ImGui::ShowDemoWindow(&showDemo);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}