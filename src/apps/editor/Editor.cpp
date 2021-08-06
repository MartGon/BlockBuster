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
    camera.SetParam(Rendering::Camera::Param::FOV, glm::radians(75.0f));
    
    // World
    mapsFolder = GetConfigOption("MapsFolder", ".");
    auto mapName = GetConfigOption("Map", "Map.bbm");
    bool mapLoaded = false;
    if(!mapName.empty() && mapName.size() < 16)
    {
        std::strcpy(fileName, mapName.c_str());
        mapLoaded = OpenMap();
    }
    
    if(!mapLoaded)
        NewMap();

    // Cursor
    cursor.show = GetConfigOption("showCursor", "1") == "1";
}

void BlockBuster::Editor::Update()
{
    // Clear Buffer
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw cubes/slopes
    for(int i = 0; i < blocks.size(); i++)
    {
        auto block = blocks[i];
        auto model = block.transform.GetTransformMat();
        auto type = block.type;
        auto transform = camera.GetProjViewMat() * model;

        shader.SetUniformInt("isPlayer", 0);
        shader.SetUniformInt("hasBorder", true);
        shader.SetUniformMat4("transform", transform);
        auto& mesh = GetMesh(block.type);
        auto display = tool == PAINT_BLOCK && preColorBlockIndex == i ? GetBlockDisplay() : block.display;
        if(display.type == Game::DisplayType::TEXTURE)
            mesh.Draw(shader, &textures[display.id]);
        else if(display.type == Game::DisplayType::COLOR)
        {
            auto color = colors[display.id];
            auto borderColor = GetBorderColor(color);
            shader.SetUniformVec4("borderColor", borderColor);
            mesh.Draw(shader, color);
        }
    }

    if(playerMode)
        UpdatePlayerMode();
    else
        UpdateEditor();

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
    config.options["MapsFolder"] = mapsFolder.string();
    config.options["showCursor"] = std::to_string(cursor.show);
}

// #### Rendering #### \\

Rendering::Mesh& BlockBuster::Editor::GetMesh(Game::BlockType blockType)
{
    return blockType == Game::BlockType::SLOPE ? slope : cube;
}

glm::vec4 BlockBuster::Editor::GetBorderColor(glm::vec4 basecolor, glm::vec4 darkColor, glm::vec4 lightColor)
{
    auto color = basecolor;
    auto darkness = (color.r + color.g + color.b) / 3.0f;
    auto borderColor = darkness <= 0.5 ? lightColor : darkColor;
    return borderColor;
}

bool BlockBuster::Editor::LoadTexture()
{
    if(textures.size() >= MAX_TEXTURES)
        return false;

    if(IsTextureInPalette(textureFolder, textureFilename))
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

bool BlockBuster::Editor::IsTextureInPalette(std::filesystem::path folder, std::filesystem::path textureName)
{
    auto texturePath = folder / textureName;
    for(const auto& texture : textures)
        if(texture.GetPath() == texturePath)
            return true;
    
    return false;
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

void BlockBuster::Editor::ResizeBlocks()
{
    for(auto& block : blocks)
    {
        block.transform.position = block.transform.position / block.transform.scale;
        block.transform.position *= blockScale;
        block.transform.scale = blockScale;
    }
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

    // Flags
    newMap = true;
    unsaved = false;

    // Filename
    std::strcpy(fileName, "NewMap.bbm");
}

void BlockBuster::Editor::SaveMap()
{
    std::filesystem::path mapPath = mapsFolder / fileName;
    std::fstream file{mapPath, file.binary | file.out};
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

    RenameMainWindow(fileName);

    // Update flag
    newMap = false;
    unsaved = false;
}

bool BlockBuster::Editor::OpenMap()
{
    std::filesystem::path mapPath = mapsFolder / fileName;
    std::fstream file{mapPath, file.binary | file.in};
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

    // Update flag
    newMap = false;
    unsaved = false;

    // Color Palette
    colorPick = colors[colorId];

    return true;
}

// #### Editor #### \\

void BlockBuster::Editor::UpdateEditor()
{
    // Handle Events    
    SDL_Event e;
    ImGuiIO& io = ImGui::GetIO();
    while(SDL_PollEvent(&e) != 0)
    {
        ImGui_ImplSDL2_ProcessEvent(&e);

        switch(e.type)
        {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            {
                if(io.WantTextInput)
                    break;

                auto keyEvent = e.key;
                HandleKeyShortCut(keyEvent);
            }
            break;
        case SDL_QUIT:
            Exit();
            break;
        case SDL_WINDOWEVENT:
            HandleWindowEvent(e.window);
            break;
        case SDL_MOUSEBUTTONDOWN:
            if(!io.WantCaptureMouse)
            {
                auto button = e.button.button;
                if(button == SDL_BUTTON_RIGHT || button == SDL_BUTTON_LEFT)
                {
                    ActionType actionType = button == SDL_BUTTON_RIGHT ? ActionType::RIGHT_BUTTON : ActionType::LEFT_BUTTON;
                    auto mousePos = GetMousePos();
                    UseTool(mousePos, actionType);
                }

                if(button == SDL_BUTTON_MIDDLE)
                {
                    SetCameraMode(CameraMode::FPS);
                }
            }
            break;
        case SDL_MOUSEBUTTONUP:
            {
                auto button = e.button.button;
                if(button == SDL_BUTTON_MIDDLE)
                {
                    SetCameraMode(CameraMode::EDITOR);
                }
            }
            break;
        case SDL_MOUSEMOTION:
            if(cameraMode == CameraMode::FPS)
                UpdateFPSCameraRotation(e.motion);
            break;
        }
    }

    // Hover
    if(!io.WantCaptureMouse)
    {
        // Reset previewColor
        preColorBlockIndex = -1;

        auto mousePos = GetMousePos();
        UseTool(mousePos, ActionType::HOVER);
    }
    
    // Camera
    if(cameraMode == CameraMode::EDITOR)
        UpdateEditorCamera();
    else if(cameraMode == CameraMode::FPS)
        UpdateFPSCameraPosition();

    // Draw Cursor
    if(cursor.enabled && cursor.show)
    {
        auto cursorTransform = cursor.transform;
        cursorTransform.scale = blockScale;
        auto model = cursorTransform.GetTransformMat();
        auto transform = camera.GetProjViewMat() * model;
        shader.SetUniformMat4("transform", transform);
        shader.SetUniformInt("hasBorder", false);
        auto& mesh = GetMesh(cursor.type);
        mesh.Draw(shader, cursor.color, GL_LINE);
    }

    // Create GUI
    GUI();
}

void BlockBuster::Editor::UpdateEditorCamera()
{
    if(io_->WantCaptureKeyboard)
        return;
    auto state = SDL_GetKeyboardState(nullptr);

    // Rotation
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
    
    // Position
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
    //if(state[SDL_SCANCODE_F])
    //    cameraPos = glm::vec3{0.0f, 2.0f, 0.0f};
           
    auto offset = glm::length(moveDir) > 0.0f ? (glm::normalize(moveDir) * CAMERA_MOVE_SPEED) : moveDir;
    cameraPos += offset;
    camera.SetPos(cameraPos);
}

void BlockBuster::Editor::UpdateFPSCameraPosition()
{
    auto state = SDL_GetKeyboardState(nullptr);

    auto cameraPos = camera.GetPos();
    auto front = camera.GetFront();
    auto xAxis = glm::normalize(-glm::cross(front, Rendering::Camera::UP));
    auto zAxis = glm::normalize(-glm::cross(xAxis, Rendering::Camera::UP));
    glm::vec3 moveDir{0};
    if(state[SDL_SCANCODE_A])
        moveDir += xAxis;
    if(state[SDL_SCANCODE_D])
        moveDir += -xAxis;
    if(state[SDL_SCANCODE_W])
        moveDir += front;
    if(state[SDL_SCANCODE_S])
        moveDir += -front;
    if(state[SDL_SCANCODE_Q])
        moveDir.y += 1.0f;
    if(state[SDL_SCANCODE_E])
        moveDir.y += -1.0f;

    auto offset = glm::length(moveDir) > 0.0f ? (glm::normalize(moveDir) * CAMERA_MOVE_SPEED) : moveDir;
    cameraPos += offset;
    camera.SetPos(cameraPos);
}

void BlockBuster::Editor::UpdateFPSCameraRotation(SDL_MouseMotionEvent motion)
{
    auto winSize = GetWindowSize();
    SDL_WarpMouseInWindow(window_, winSize.x / 2, winSize.y / 2);

    auto cameraRot = camera.GetRotation();
    auto pitch = cameraRot.x;
    auto yaw = cameraRot.y;

    pitch = glm::max(glm::min(pitch + motion.yrel * CAMERA_ROT_SPEED  / 10.0f, glm::pi<float>() - CAMERA_ROT_SPEED), CAMERA_ROT_SPEED);
    yaw = yaw - motion.xrel * CAMERA_ROT_SPEED / 10.0f;

    camera.SetRotation(pitch, yaw);
}

void BlockBuster::Editor::SetCameraMode(CameraMode mode)
{
    cameraMode = mode;
    auto& io = ImGui::GetIO();
    if(cameraMode == CameraMode::FPS)
    {
        io.ConfigFlags |= ImGuiConfigFlags_::ImGuiConfigFlags_NoMouseCursorChange;
        SDL_SetWindowGrab(window_, SDL_TRUE);
        SDL_SetRelativeMouseMode(SDL_TRUE);
    }
    else
    {
        io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
        SDL_SetWindowGrab(window_, SDL_FALSE);
        SDL_SetRelativeMouseMode(SDL_FALSE);

        auto winSize = GetWindowSize();
        SDL_WarpMouseInWindow(window_, winSize.x / 2, winSize.y / 2);
    }
}

void BlockBuster::Editor::UseTool(glm::vec<2, int> mousePos, ActionType actionType)
{
    // Window to eye
    auto winSize = GetWindowSize();
    auto ray = Rendering::ScreenToWorldRay(camera, mousePos, glm::vec2{winSize.x, winSize.y});
    if(cameraMode == CameraMode::FPS)
        ray = Collisions::Ray{camera.GetPos(), camera.GetPos() + camera.GetFront()};

    // Sort blocks by proximity to camera
    auto cameraPos = this->camera.GetPos();
    std::sort(blocks.begin(), blocks.end(), [cameraPos](Game::Block a, Game::Block b)
    {
        auto toA = glm::length(a.transform.position - cameraPos);
        auto toB = glm::length(b.transform.position - cameraPos);
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
    
    // Use appropiate Tool
    switch(tool)
    {
        case PLACE_BLOCK:
        {
            if(index != -1)
            {
                if(actionType == ActionType::LEFT_BUTTON || actionType == ActionType::HOVER)
                {
                    auto block = blocks[index];
                    auto pos = block.transform.position;
                    auto scale = block.transform.scale;

                    auto newBlockPos = pos + intersection.normal * scale;
                    if(!GetBlock(newBlockPos))
                    {
                        auto yaw = blockType == Game::BlockType::SLOPE ? (std::round(glm::degrees(camera.GetRotation().y) / 90.0f) * 90.0f) - 90.0f : 0.0f;
                        auto newBlockRot = glm::vec3{0.0f, yaw, 0.0f};

                        if(actionType == ActionType::LEFT_BUTTON)
                        {
                            auto display = GetBlockDisplay();                            
                            auto newBlockType = blockType;
                            blocks.push_back({Math::Transform{newBlockPos, newBlockRot, scale}, newBlockType, display});

                            SetUnsaved(true);
                        }
                        else if(actionType == ActionType::HOVER)
                        {
                            cursor.enabled = true;
                            cursor.transform.position = newBlockPos;
                            cursor.transform.rotation = newBlockRot;
                            cursor.color = yellow;
                            cursor.type = blockType;
                            if(block.display.type == Game::DisplayType::COLOR)
                            {
                                cursor.color = GetBorderColor(colors[block.display.id], darkBlue, yellow);
                            }
                        }
                    }
                    else
                    {
                        // Disable cursor
                        cursor.enabled = false;
                    }
                }
                else if(actionType == ActionType::RIGHT_BUTTON)
                {
                    if(blocks.size() > 1)
                    {
                        blocks.erase(blocks.begin() + index);
                        SetUnsaved(true);
                    }

                    break;
                }
            }
            else
            {
                cursor.enabled = false;
            }
            break;
        }               
        case ROTATE_BLOCK:
        {
            if(index != -1)
            {
                auto& block = blocks[index];
                if(actionType == ActionType::LEFT_BUTTON || actionType == ActionType::RIGHT_BUTTON)
                {
                    int mod = actionType == ActionType::RIGHT_BUTTON ? -90 : 90;
                    if(block.type == Game::BlockType::SLOPE)
                    {
                        glm::ivec3 rot = block.transform.rotation;
                        if(axis == RotationAxis::X)
                            rot.x += mod;
                        else if(axis == RotationAxis::Y)
                            rot.y += mod;
                        else if(axis == RotationAxis::Z)
                            rot.z = ((rot.z + mod) % 270);

                        block.transform.rotation = rot;
                        SetUnsaved(true);
                    }
                }
                if(actionType == ActionType::HOVER)
                {
                    if(block.type == Game::BlockType::SLOPE)
                    {
                        cursor.enabled = true;
                        cursor.transform.position = block.transform.position;
                        cursor.type = Game::BlockType::BLOCK;
                        if(block.display.type == Game::DisplayType::COLOR)
                        {
                            cursor.color = GetBorderColor(colors[block.display.id], darkBlue, yellow);
                        }
                    }
                    else
                        cursor.enabled = false;
                }
            }
            else
                cursor.enabled = false;

            break;
        }
        case PAINT_BLOCK:
        {
            if(index != -1)
            {
                auto& block = blocks[index];
                if(actionType == ActionType::LEFT_BUTTON)
                {
                    block.display = GetBlockDisplay();
                    SetUnsaved(true);
                }
                if(actionType == ActionType::RIGHT_BUTTON)
                {
                    SetBlockDisplay(block.display);
                }
                if(actionType == ActionType::HOVER)
                {
                    std::cout << "Hovering over block " << index << "\n";
                    preColorBlockIndex = index;
                }
            }
            break;
        }
    }
}

void BlockBuster::Editor::HandleKeyShortCut(const SDL_KeyboardEvent& key)
{
    auto& io = ImGui::GetIO();
    if(key.type == SDL_KEYDOWN)
    {
        auto sym = key.keysym.sym;
        if(sym == SDLK_ESCAPE)
            Exit();

        if(sym == SDLK_f)
        {
            auto mode = cameraMode == CameraMode::EDITOR ? CameraMode::FPS : CameraMode::EDITOR;
            SetCameraMode(mode);
        }
        
        if(sym == SDLK_p)
        {
            playerMode = !playerMode;
            std::cout << "Player mode enabled: " << playerMode << "\n";
            if(playerMode)
            {
                player.transform.position = camera.GetPos();
                SetCameraMode(CameraMode::FPS);
            }
        }

        if(sym == SDLK_n && io.KeyCtrl)
            MenuNewMap();

        if(sym == SDLK_s && io.KeyCtrl)
            MenuSave();

        if(sym == SDLK_s && io.KeyCtrl && io.KeyShift)
            MenuSaveAs();

        if(sym == SDLK_o && io.KeyCtrl)
            MenuOpenMap();

        if(sym >= SDLK_1 &&  sym <= SDLK_3 && io.KeyCtrl)
            tool = static_cast<Tool>(sym - SDLK_1);

        if(sym >= SDLK_1 && sym <= SDLK_2 && !io.KeyCtrl && !io.KeyAlt)
        {
            if(tool == Tool::PLACE_BLOCK)
                blockType = static_cast<Game::BlockType>(sym - SDLK_1);
            if(tool == Tool::ROTATE_BLOCK)
                axis = static_cast<RotationAxis>(sym - SDLK_0);
        }

        if(sym >= SDLK_1 && sym <= SDLK_2 && !io.KeyCtrl && io.KeyAlt)
            tabState = static_cast<TabState>(sym - SDLK_1)        ;
        
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

void BlockBuster::Editor::SetUnsaved(bool unsaved)
{
    this->unsaved = unsaved;
    if(unsaved)
        RenameMainWindow(fileName + std::string("*"));
    else
        RenameMainWindow(fileName);
}

void BlockBuster::Editor::OpenWarningPopUp(std::function<void()> onExit)
{
    state = PopUpState::UNSAVED_WARNING;
    onWarningExit = onExit;
}

void BlockBuster::Editor::Exit()
{
    if(unsaved)
    {
        if(playerMode)
            playerMode = false;
        OpenWarningPopUp(std::bind(&BlockBuster::Editor::Exit, this));
        SetCameraMode(CameraMode::EDITOR);
    }
    else
        quit = true;
}

// #### Test Mode #### \\

void BlockBuster::Editor::UpdatePlayerMode()
{
    SDL_Event e;
    ImGuiIO& io = ImGui::GetIO();
    while(SDL_PollEvent(&e) != 0)
    {
        ImGui_ImplSDL2_ProcessEvent(&e);

        switch(e.type)
        {
        case SDL_KEYDOWN:
            if(e.key.keysym.sym == SDLK_ESCAPE)
                Exit();
            if(e.key.keysym.sym == SDLK_p)
            {
                playerMode = !playerMode;
                std::cout << "Player mode enabled: " << playerMode << "\n";
                if(playerMode)
                {
                    player.transform.position = camera.GetPos();
                    SetCameraMode(CameraMode::FPS);
                }
                else
                    SetCameraMode(CameraMode::EDITOR);
            }
            break;
        case SDL_QUIT:
            Exit();
            break;
        case SDL_WINDOWEVENT:
            HandleWindowEvent(e.window);
            break;
        case SDL_MOUSEMOTION:
            if(cameraMode == CameraMode::FPS)
                UpdateFPSCameraRotation(e.motion);
            break;
        }
    }

    player.transform.rotation = glm::vec3{0.0f, glm::degrees(camera.GetRotation().y) - 90.0f, 0.0f};

    player.Update();
    player.HandleCollisions(blocks);
    auto playerPos = player.transform.position;

    auto cameraPos = playerPos + glm::vec3{0.0f, 2.f, 0.0f};
    camera.SetPos(cameraPos);
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
    std::string errorPrefix = "Could not open map ";
    auto onAccept = std::bind(&BlockBuster::Editor::OpenMap, this);
    auto onCancel = [](){};
    BasicPopUpParams params{PopUpState::OPEN_MAP, "Open Map", fileName, 32, onAccept, onCancel, errorPrefix, onError, errorText};
    EditTextPopUp(params);
}

void BlockBuster::Editor::SaveAsPopUp()
{
    std::string errorPrefix = "Could not save map ";
    auto onAccept = [this](){this->SaveMap(); return true;};
    auto onCancel = [](){};
    BasicPopUpParams params{PopUpState::SAVE_AS, "Save As", fileName, 32, onAccept, onCancel, errorPrefix, onError, errorText};
    EditTextPopUp(params);
}

void BlockBuster::Editor::LoadTexturePopUp()
{
    std::string errorPrefix = "Could not open texture ";
    auto onAccept = std::bind(&BlockBuster::Editor::LoadTexture, this);
    auto onCancel = [](){};
    BasicPopUpParams params{PopUpState::LOAD_TEXTURE, "Load Texture", textureFilename, 32, onAccept, onCancel, errorPrefix, onError, errorText};
    EditTextPopUp(params);
}

void BlockBuster::Editor::EditTextPopUp(const BasicPopUpParams& params)
{
    if(state == params.popUpState)
        ImGui::OpenPopup(params.name.c_str());

    auto displaySize = io_->DisplaySize;
    ImGui::SetNextWindowPos(ImVec2{displaySize.x * 0.5f, displaySize.y * 0.5f}, ImGuiCond_Always, ImVec2{0.5f, 0.5f});
    if(ImGui::BeginPopupModal(params.name.c_str(), nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
    {   
        bool accept = ImGui::InputText("File name", params.textBuffer, params.bufferSize, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
        bool ok = true;
        if(ImGui::Button("Accept") || accept)
        {
            ok = params.onAccept();

            if(ok)
            {
                params.onError = false;
                
                ImGui::CloseCurrentPopup();
                state = PopUpState::NONE;
            }
            else
            {
                params.onError = true;
                params.errorText = params.errorPrefix + params.textBuffer + '\n';
            }
        }

        ImGui::SameLine();
        if(ImGui::Button("Cancel"))
        {
            params.onCancel();

            ImGui::CloseCurrentPopup();
            state = PopUpState::NONE;
        }

        if(params.onError)
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", params.errorText.c_str());

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

void BlockBuster::Editor::UnsavedWarningPopUp()
{
    if(state == PopUpState::UNSAVED_WARNING)
        ImGui::OpenPopup("Unsaved content");

    auto displaySize = io_->DisplaySize;
    ImGui::SetNextWindowPos(ImVec2{displaySize.x * 0.5f, displaySize.y * 0.5f}, ImGuiCond_Always, ImVec2{0.5f, 0.5f});

    bool onPopUp = state == PopUpState::UNSAVED_WARNING;
    if(ImGui::BeginPopupModal("Unsaved content", &onPopUp, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("There's unsaved content.\nWhat would you like to do?");

        if(ImGui::Button("Save"))
        {
            SaveMap();
            state = PopUpState::NONE;
            onWarningExit();
        }
        ImGui::SameLine();

        if(ImGui::Button("Don't Save"))
        {
            unsaved = false;
            state = PopUpState::NONE;

            onWarningExit();
        }
        ImGui::SameLine();

        if(ImGui::Button("Cancel"))
        {
            state = PopUpState::NONE;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
    else if(state == PopUpState::UNSAVED_WARNING)
    {
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
                MenuNewMap();
            }

            ImGui::Separator();

            if(ImGui::MenuItem("Open Map", "Ctrl + O"))
            {
                MenuOpenMap();
            }

            ImGui::Separator();

            if(ImGui::MenuItem("Save", "Ctrl + S"))
            {
                MenuSave();
            }

            if(ImGui::MenuItem("Save As", "Ctrl + Shift + S"))
            {
                MenuSaveAs();
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

void BlockBuster::Editor::MenuNewMap()
{
    if(unsaved)
    {
        state = PopUpState::UNSAVED_WARNING;
        auto onExit = std::bind(&BlockBuster::Editor::NewMap, this);
        OpenWarningPopUp(onExit);
    }
    else
        NewMap();
}

void BlockBuster::Editor::MenuOpenMap()
{
    if(unsaved)
    {
        state = PopUpState::UNSAVED_WARNING;
        auto onExit = [this](){this->state = PopUpState::OPEN_MAP;};
        OpenWarningPopUp(onExit);
    }
    else
        state = PopUpState::OPEN_MAP;
}

void BlockBuster::Editor::MenuSave()
{
    if(newMap)
        state = PopUpState::SAVE_AS;
    else
        SaveMap();
}

void BlockBuster::Editor::MenuSaveAs()
{
    state = PopUpState::SAVE_AS;
}

void BlockBuster::Editor::GUI()
{
    // Clear GUI buffer
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
        UnsavedWarningPopUp();
        MenuBar();

        if(ImGui::BeginTabBar("Tabs"))
        {
            auto flags = tabState == TabState::TOOLS_TAB ? ImGuiTabItemFlags_SetSelected : 0;
            if(ImGui::BeginTabItem("Tools", nullptr, flags))
            {
                if(ImGui::IsItemActive())
                    tabState = TabState::TOOLS_TAB;

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

                        ImGui::Checkbox("Show cursor", &cursor.show);
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

                                // This part of the code is to "fix" the display of the palette
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
                                // Until around here
                                
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
                                    GL::Texture* texture = &textures[i];
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
                            ImGui::SameLine();
                            ImGui::Text("Select color");
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
                        ImGui::RadioButton("Y", &axis, RotationAxis::Y);
                        ImGui::SameLine();
                        ImGui::RadioButton("Z", &axis, RotationAxis::Z);
                        ImGui::Checkbox("Show cursor", &cursor.show);
                    }

                    ImGui::EndTable();
                }

                ImGui::EndTabItem();
            }
            
            flags = tabState == TabState::OPTIONS_TAB ? ImGuiTabItemFlags_SetSelected : 0;
            if(ImGui::BeginTabItem("Options", nullptr, flags))
            {
                if(ImGui::IsItemActive())
                    tabState = TabState::OPTIONS_TAB;

                if(ImGui::SliderFloat("Block Scale", &blockScale, 1, 5))
                {
                    ResizeBlocks();
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }

    ImGui::End();

    if(showDemo)
        ImGui::ShowDemoWindow(&showDemo);

    // Draw GUI
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}