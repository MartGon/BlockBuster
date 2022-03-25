#include <Editor.h>

#include <entity/Game.h>
#include <math/BBMath.h>
#include <game/Input.h>

#include <iostream>
#include <algorithm>
#include <fstream>
#include <cstring>

#include <glm/gtc/constants.hpp>

#include <debug/Debug.h>

#include <imgui/backends/imgui_impl_opengl3.h>

#define TRY_LOAD(X) \
  try{ \
    (X); \
  } \
  catch(const std::runtime_error& e){ this->logger->LogError(e.what());} 

using namespace BlockBuster::Editor;

// #### Public Interface #### \\

void Editor::Start()
{
    // Load shaders
    try{
        shader = GL::Shader::FromFolder(config.openGL.shadersFolder, "simpleVertex.glsl", "simpleFrag.glsl");
        paintShader = GL::Shader::FromFolder(config.openGL.shadersFolder, "paintVertex.glsl", "paintFrag.glsl");
        chunkShader = GL::Shader::FromFolder(config.openGL.shadersFolder, "chunkVertex.glsl", "chunkFrag.glsl");
        quadShader = GL::Shader::FromFolder(config.openGL.shadersFolder, "quadVertex.glsl", "quadFrag.glsl");
        skyboxShader = GL::Shader::FromFolder(config.openGL.shadersFolder, "skyboxVertex.glsl", "skyboxFrag.glsl");
    }
    catch(const std::runtime_error& e)
    {
        this->logger->LogCritical(e.what());
        quit = true;
        return;
    }

    // Textures
    std::filesystem::path texturesDir = TEXTURES_DIR;
    GL::Cubemap::TextureMap map = {
        {GL::Cubemap::RIGHT, texturesDir / "right.jpg"},
        {GL::Cubemap::LEFT, texturesDir / "left.jpg"},
        {GL::Cubemap::TOP, texturesDir / "top.jpg"},
        {GL::Cubemap::BOTTOM, texturesDir / "bottom.jpg"},
        {GL::Cubemap::FRONT, texturesDir / "front.jpg"},
        {GL::Cubemap::BACK, texturesDir / "back.jpg"},
    };
    TRY_LOAD(skybox.Load(map, false));

    TRY_LOAD(flashTexture.LoadFromFolder(TEXTURES_DIR, "flash.png"));

    // Meshes
    cube = Rendering::Primitive::GenerateCube();
    slope = Rendering::Primitive::GenerateSlope();
    cylinder = Rendering::Primitive::GenerateCylinder(1.f, 1.f, 16, 1);

    // Models
    modelMgr.Start(renderMgr, shader);

    respawnModel.SetMeshes(cylinder, slope);
    respawnModel.Start(renderMgr, shader);

    playerAvatar.SetMeshes(modelMgr.quad, modelMgr.cube, modelMgr.cylinder, modelMgr.slope);
    playerAvatar.Start(renderMgr, shader, quadShader, flashTexture);
    modelMgr.SetModel(Entity::GameObject::Type::PLAYER_DECOY, &playerAvatar);
    
    // OpenGL features
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE);
   
    chunkShader.SetUniformInt("textureArray", 0);
    chunkShader.SetUniformInt("colorArray", 1);

    // Camera
    int width, height;
    SDL_GetWindowSize(window_, &width, &height);
    camera.SetParam(Rendering::Camera::Param::ASPECT_RATIO, (float)width / (float)height);
    camera.SetParam(Rendering::Camera::Param::FOV, config.window.fov);
    cameraController = ::App::Client::CameraController{&camera, {window_, io_}, ::App::Client::CameraMode::EDITOR};
    cameraController.moveSpeed = std::stof(GetConfigOption("camMoveSpeed", "0.25"));
    cameraController.rotSpeed = std::stof(GetConfigOption("camRotSpeed", "0.017"));
    
    // World
    mapsFolder = GetConfigOption("MapsFolder", "./maps/");
    mapMgr.SetMapsFolder(mapsFolder);

    gui.fileName = GetConfigOption("Map", "");

    bool mapLoaded = false;
    if(!gui.fileName.empty())
    {
        mapLoaded = OpenProject().type == Util::ResultType::SUCCESS;
    }
    
    if(!mapLoaded)
        NewProject();

    // Init template go
    placedGo = Entity::GameObject::Create(Entity::GameObject::Type::RESPAWN);

    // Cursor
    cursor.show = GetConfigOption("showCursor", "1") == "1";

    // GUI
    gui.InitPopUps();
}

void Editor::Update()
{
    // Clear Buffer
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Setup texture arrays
    project.map.tPalette.GetTextureArray()->Bind(GL_TEXTURE0);
    project.map.cPalette.GetTextureArray()->Bind(GL_TEXTURE1);

    // Draw new Map System Cubes
    project.map.Draw(chunkShader, camera.GetProjViewMat());

        // Draw skybox
    auto view = camera.GetViewMat();
    auto proj = camera.GetProjMat();
    skybox.Draw(skyboxShader, view, proj, true);
    
    if(playerMode)
        UpdatePlayerMode();
    else
        UpdateEditor();

    SDL_GL_SwapWindow(window_);
}

bool Editor::Quit()
{
    return quit;
}

void Editor::Shutdown()
{
    config.options["Map"] = gui.fileName;
    config.options["MapsFolder"] = mapsFolder.string();
    config.options["showCursor"] = std::to_string(cursor.show);
    config.options["camMoveSpeed"] = std::to_string(cameraController.moveSpeed);
    config.options["camRotSpeed"] = std::to_string(cameraController.rotSpeed);
}

// #### Rendering #### \\

Rendering::Mesh& Editor::GetMesh(Game::BlockType blockType)
{
    return blockType == Game::BlockType::SLOPE ? slope : cube;
}

glm::vec4 Editor::GetBorderColor(glm::vec4 basecolor, glm::vec4 darkColor, glm::vec4 lightColor)
{
    auto color = basecolor;
    auto darkness = (color.r + color.g + color.b) / 3.0f;
    auto borderColor = darkness <= 0.5 ? lightColor : darkColor;
    return borderColor;
}

Util::Result<bool> Editor::LoadTexture()
{
    if(project.map.tPalette.GetCount() >= gui.MAX_TEXTURES)
        return Util::CreateError<bool>("Maximum of textures reached");

    auto textureFolder = mapMgr.GetMapFolder(gui.fileName) / "textures";
    if(IsTextureInPalette(textureFolder, gui.textureFilename))
        return Util::CreateError<bool>("Texture is already in palette");

    auto res = project.map.tPalette.AddTexture(textureFolder, gui.textureFilename, false);
    if(res.type == Util::ResultType::ERROR)
    {
        return Util::CreateError<bool>(res.err.info);
    }
    else
    {
        gui.SyncGUITextures();

        auto count = project.map.tPalette.GetCount();
        gui.placeBlock.display.id = project.map.tPalette.GetMember(count - 1).data.id;
    }

    return Util::CreateSuccess<bool>(true);
}

bool Editor::IsTextureInPalette(std::filesystem::path folder, std::filesystem::path textureName)
{
    auto texturePath = folder / textureName;
    for(int i = 0; i < project.map.tPalette.GetCount(); i++)
        if(project.map.tPalette.GetMember(i).data.filepath == texturePath)
            return true;
    
    return false;
}

void Editor::ResetTexturePalette()
{
    project.map.tPalette = Rendering::TexturePalette{16};

    // Set to first color
    auto it = this->project.map.GetMap()->CreateIterator();
    for(auto [pos, block] = it.GetNextBlock(); !it.IsOver(); std::tie(pos, block) = it.GetNextBlock())
    {
        if(block && block->display.type == Game::DisplayType::TEXTURE)
        {
            auto replace = *block;
            replace.display.type = Game::DisplayType::COLOR;
            replace.display.id = 0;
            this->project.map.SetBlock(pos, replace);
        }
    }

    gui.SyncGUITextures();
}

// #### World #### \\

void Editor::NewProject()
{
    // Init project
    project = Project{};
    project.Init();
    gui.SyncGUITextures();

    // Camera
    camera.SetPos(glm::vec3 {0.0f, 6.0f, 6.0f});
    camera.SetTarget(glm::vec3{0.0f});

    // Filename
    gui.fileName = "NewMap";

    // Window
    RenameMainWindow();

    // Flags
    newMap = true;
    unsaved = false;

    ClearActionHistory();
}

void Editor::SaveProject()
{
    std::filesystem::path mapFolder = mapsFolder / gui.fileName;

    if(!std::filesystem::is_directory(mapFolder))
    {
        std::filesystem::create_directory(mapFolder);
        auto texturesFolder = mapFolder / "textures";
        std::filesystem::create_directory(texturesFolder);
    }
    std::string fileName = gui.fileName + ".bbm";

    // Write project file
    // Camera
    project.cameraPos = camera.GetPos();
    project.cameraRot = camera.GetRotation();

    // Cursor Pos
    project.cursorPos = cursor.pos;
    project.cursorScale = cursor.scale;
    ::WriteProjectToFile(project, mapFolder, fileName);

    // Update flag
    newMap = false;
    unsaved = false;

    RenameMainWindow();
}

Util::Result<bool> Editor::OpenProject()
{
    auto mapFolder = mapMgr.GetMapFolder(gui.fileName);
    std::string fileName = gui.fileName + ".bbm";

    auto res = Util::CreateError<bool>("Could not open project");

    Project temp = ::ReadProjectFromFile(mapFolder, fileName);
    bool isOk = temp.isOk;
    if(isOk)
    {
        // Move to main project if it's ok
        project = std::move(temp);
        gui.SyncGUITextures();

        // Update flag
        newMap = false;
        unsaved = false;

        // Window
        RenameMainWindow();

        // Color Palette
        gui.colorPick = Rendering::Uint8ColorToFloat(project.map.cPalette.GetMember(0).data.color);

        // Clear history
        ClearActionHistory();

        // Set camera pos
        camera.SetPos(project.cameraPos);
        camera.SetRotation(project.cameraRot.x, project.cameraRot.y);

        // Set cursor
        cursor.pos = project.cursorPos;
        savedPos = project.cursorPos;
        cursor.scale = project.cursorScale;

        // Blockscale
        blockScale = project.map.GetBlockScale();

        res = Util::CreateSuccess<bool>(true);
    }
    
    
    return res;
}

// #### Editor #### \\

void Editor::UpdateEditor()
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
                    SetCameraMode(::App::Client::CameraMode::FPS);
                }
            }
            break;
        case SDL_MOUSEBUTTONUP:
            {
                auto button = e.button.button;
                if(button == SDL_BUTTON_MIDDLE)
                {
                    SetCameraMode(::App::Client::CameraMode::EDITOR);
                }
            }
            break;
        case SDL_MOUSEMOTION:
            cameraController.HandleSDLEvent(e);
            break;
        }
    }

    // Hover or Hold
    if(!io.WantCaptureMouse)
    {
        auto mousePos = GetMousePos();
        auto mouseState = SDL_GetMouseState(nullptr, nullptr);
        auto actionType = ActionType::HOVER;
        if(mouseState & SDL_BUTTON_LEFT)
            actionType = ActionType::HOLD_LEFT_BUTTON;
        else if(mouseState & SDL_BUTTON_RIGHT)
            actionType = ActionType::HOLD_RIGHT_BUTTON;

        UseTool(mousePos, actionType);
    }
    
    // Camera
    auto view = camera.GetProjViewMat();
    if(!io_->WantCaptureKeyboard)
    {
        cameraController.Update();
    }
    
    auto map = project.map.GetMap();
    auto blockScale = project.map.GetBlockScale();

    // Render respawn models
    auto rIndices = map->GetRespawnIndices();
    for(auto pos : rIndices)
    {
        // Find real pos
        auto rPos = Game::Map::ToRealPos(pos, blockScale);
        rPos.y -= (blockScale / 2.0f);

        auto respawn = map->GetRespawn(pos);
        Math::Transform t{rPos, glm::vec3{0.0f, respawn->orientation, 0.0f}, glm::vec3{0.8f}};
        auto tMat = view * t.GetTransformMat();
        respawnModel.Draw(tMat);
    }

    // Render object models
    auto goIndices = map->GetGameObjectIndices();
    for(auto goPos : goIndices)
    {
        auto go = map->GetGameObject(goPos);
        auto rPos = Game::Map::ToRealPos(goPos, blockScale);
        rPos.y -= (blockScale / 2.0f);

        Math::Transform t{rPos, glm::vec3{0.0f}, glm::vec3{1.0f}};
        if(go->type !=  Entity::GameObject::Type::PLAYER_DECOY)
        {
            auto tMat = view * t.GetTransformMat();
            modelMgr.Draw(go->type, tMat);
        }
        else
        {
            auto ecb = player.GetECB();
            t.position.y = t.position.y + ecb.scale.y / 2.0f;
            t.scale = glm::vec3{Entity::Player::scale};
            auto tMat = view * t.GetTransformMat();
            modelMgr.Draw(go->type, tMat);
        }
        
        // Special cases
        if(go->type == Entity::GameObject::DOMINATION_POINT && gui.selectedObj == goPos)
        {
            float scale = std::get<float>(go->properties["Scale"].value);
            t.position.y += ((3.0f * blockScale) / 2.0f);
            t.scale = glm::vec3{scale, 3.0f, scale} * blockScale;
            DrawCursor(t);
        }
    }

    // Draw Cursor
    glDisable(GL_DEPTH_TEST);
    if(cursor.enabled && cursor.show && tool != SELECT_BLOCKS)
    {   
        auto t = Game::GetBlockTransform(cursor.pos, cursor.rot, project.map.GetBlockScale());
        DrawCursor(t);
    }
    else if(tool == SELECT_BLOCKS)
    {
        DrawSelectCursor(cursor.pos);
    }
    glEnable(GL_DEPTH_TEST);

    // Draw pre view painted block
    bool isPainted = intersecting && tool == PAINT_BLOCK;
    if(isPainted)
    {
        auto block = project.map.GetBlock(pointedBlockPos);
        Math::Transform t = Game::GetBlockTransform(block, pointedBlockPos, project.map.GetBlockScale());

        // HACK: To avoid z fighting.
        // Option 2: Changing block display each time the block is pointed/unpointd. Create wrapper function to do it safely. 
        //  Revert changes when saving project
        const auto factor = 1.025f;
        t.scale *= factor;

        if(block.type == Game::BlockType::SLOPE)
        {
            const glm::vec3 up{0.0f, 1.0f, 0.0f};
            glm::vec3 rot = t.GetRotationMat() * glm::vec4{up, 1.0f};
            auto offset = (factor - 1.f) / 2.0f;
            t.position += (rot * offset);
        }
        
        auto mMat = t.GetTransformMat();
        auto tMat = view * mMat;
        auto& mesh = GetMesh(block.type);

        auto display =  GetBlockDisplay();
        bool validTex = display.type == Game::DisplayType::TEXTURE && display.id < project.map.tPalette.GetCount();
        bool validCol = display.type == Game::DisplayType::COLOR && display.id < project.map.cPalette.GetCount();
        bool validDisplay = validTex || validCol;

        paintShader.SetUniformMat4("transform", tMat);
        paintShader.SetUniformInt("textureType", display.type);
        paintShader.SetUniformInt("textureId", display.id);
        paintShader.SetUniformInt("overrideColor", false);

        paintShader.SetUniformInt("textureArray", 0);
        paintShader.SetUniformInt("colorArray", 1);

        if(validDisplay)
            mesh.Draw(paintShader, project.map.tPalette.GetTextureArray(), display.id);
    }

    // Draw chunk borders
    const glm::vec4 yellow = glm::vec4{1.0f, 1.0f, 0.0f, 1.0f};
    if(gui.drawChunkBorders)
        project.map.DrawChunkBorders(shader, cube, view, yellow);

    renderMgr.Render(camera);

    // Create GUI
    gui.GUI();
}

void Editor::SetCameraMode(::App::Client::CameraMode mode)
{
    cameraController.SetMode(mode);
}

void Editor::HandleKeyShortCut(const SDL_KeyboardEvent& key)
{
    auto& io = ImGui::GetIO();
    if(key.type == SDL_KEYDOWN)
    {
        auto sym = key.keysym.sym;
        if(sym == SDLK_ESCAPE)
        {
            if(gui.IsAnyPopUpOpen())
                gui.ClosePopUp();
            else
                Exit();
        }

        if(gui.IsAnyPopUpOpen())
            return;

        // Select tool
        if(tool == Tool::SELECT_BLOCKS)
        {
            auto nextPos = cursor.pos;
            auto scale = cursor.scale;

            auto moveOrScale = [&nextPos, &scale, &sym, this](Sint32 key, glm::ivec3 offset, float yawDeg)
            {
                if(sym == key)
                {
                    auto rotMat = glm::rotate(glm::mat4{1}, glm::radians(yawDeg), glm::vec3{0.0f, 1.0f, 0.0f});
                    offset = rotMat * glm::vec4{offset, 1.0f};

                    if(!this->io_->KeyCtrl)
                    {
                        nextPos += offset;
                    }
                    else
                        scale += offset;
                }
            };

            auto camYaw = camera.GetRotationDeg().y;
            auto rotAngle = (glm::round((camYaw / 90.0f)) - 1) * 90.0f;

            // X axis
            moveOrScale(SDLK_KP_4, glm::ivec3{-1, 0, 0}, rotAngle);
            moveOrScale(SDLK_KP_6, glm::ivec3{1, 0, 0}, rotAngle);

            // Y axis
            moveOrScale(SDLK_KP_7, glm::ivec3{0, 1, 0}, 0.0f);
            moveOrScale(SDLK_KP_9, glm::ivec3{0, -1, 0}, 0.0f);

            // Z axis
            moveOrScale(SDLK_KP_8, glm::ivec3{0, 0, -1}, rotAngle);
            moveOrScale(SDLK_KP_2, glm::ivec3{0, 0, 1}, rotAngle);

            bool hasMoved = nextPos != cursor.pos;
            if(!movingSelection)
                cursor.scale = glm::max(scale, glm::ivec3{1, 1, 1});
            if(hasMoved)
                MoveSelectionCursor(nextPos);
        }

        // Camera mode
        if(sym == SDLK_f)
        {
            auto mode = cameraController.GetMode() == ::App::Client::CameraMode::EDITOR ? ::App::Client::CameraMode::FPS : ::App::Client::CameraMode::EDITOR;
            SetCameraMode(mode);
        }
        
        // Editor/Player Mode toggle
        if(sym == SDLK_p)
        {
            playerMode = !playerMode;
            logger->LogDebug("Player mode enabled: " + std::to_string(playerMode));
            if(playerMode)
            {
                //player.transform.position = camera.GetPos();
                SetCameraMode(::App::Client::CameraMode::FPS);
            }
        }

        // File Menu
        if(sym == SDLK_n && io.KeyCtrl)
            gui.MenuNewMap();

        if(sym == SDLK_s && io.KeyCtrl)
            gui.MenuSave();

        if(sym == SDLK_s && io.KeyCtrl && io.KeyShift)
            gui.MenuSaveAs();

        if(sym == SDLK_o && io.KeyCtrl)
            gui.MenuOpenMap();

        // Edit
        if(sym == SDLK_z && io.KeyCtrl && !io.KeyShift)
            UndoToolAction();

        if(sym == SDLK_z && io.KeyCtrl && io.KeyShift)
            DoToolAction();

        if(sym == SDLK_g && io.KeyCtrl)
        {
            gui.OpenPopUp(EditorGUI::PopUpState::GO_TO_BLOCK);
        }

        // Editor navigation
        if(sym >= SDLK_1 &&  sym <= SDLK_4 && io.KeyCtrl)
            SelectTool(static_cast<Tool>(sym - SDLK_1));

        // Select display/blocktype/axis
        if(sym >= SDLK_1 && sym <= SDLK_2 && !io.KeyCtrl && !io.KeyAlt)
        {
            if(tool == Tool::PLACE_BLOCK)
                gui.placeBlock.type = static_cast<Game::BlockType>(sym - SDLK_0);
            if(tool == Tool::ROTATE_BLOCK)
                gui.axis = static_cast<Game::RotationAxis>(sym - SDLK_0);
            if(tool == Tool::PAINT_BLOCK)
                gui.placeBlock.display.type = static_cast<Game::DisplayType>(sym - SDLK_1);
        }

        if(sym >= SDLK_1 && sym <= SDLK_3 && !io.KeyCtrl && io.KeyAlt)
            gui.tabState = static_cast<EditorGUI::TabState>(sym - SDLK_1);
    }
}

void Editor::HandleWindowEvent(SDL_WindowEvent winEvent)
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

void Editor::RenameMainWindow()
{
    std::string mapName = !gui.fileName.empty() > 0 ? gui.fileName : "New Map";
    std::string title = "Editor - " + mapName;
    if(unsaved)
        title = title + "*";
    
    AppI::RenameMainWindow(title);
}

void Editor::SetUnsaved(bool unsaved)
{
    this->unsaved = unsaved;
    RenameMainWindow();
}

void Editor::Exit()
{
    if(unsaved)
    {
        if(playerMode)
            playerMode = false;

        gui.OpenWarningPopUp(std::bind(&Editor::Exit, this));
        if(cameraController.GetMode() != ::App::Client::CameraMode::EDITOR)
            SetCameraMode(::App::Client::CameraMode::EDITOR);
    }
    else
        quit = true;
}

// #### Test Mode #### \\

void Editor::UpdatePlayerMode()
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
                logger->LogDebug("Player mode enabled: " + std::to_string(playerMode));
                if(playerMode)
                {
                    SetCameraMode(::App::Client::CameraMode::FPS);
                }
                else
                    SetCameraMode(::App::Client::CameraMode::EDITOR);
            }
            break;
        case SDL_QUIT:
            Exit();
            break;
        case SDL_WINDOWEVENT:
            HandleWindowEvent(e.window);
            break;
        case SDL_MOUSEMOTION:
            cameraController.HandleSDLEvent(e);
            break;
        }
    }

    const auto camOffset = glm::vec3{0.0f, Entity::Player::camHeight, 0.0f};
    auto pos = camera.GetPos() - camOffset;
    auto yaw = camera.GetRotationDeg().y;
    auto input = Input::GetPlayerInput(Entity::PlayerInput{true});
    auto playerPos = player.UpdatePosition(pos, yaw, input, project.map.GetMap(), Util::Time::Seconds{0.016666f}) + camOffset;
    camera.SetPos(playerPos);
}


// #### Tools General #### \\

void Editor::SelectTool(Tool newTool)
{
    // OnDeselect
    switch (tool)
    {
    case Tool::SELECT_BLOCKS:
        movingSelection = false;
        savedPos = cursor.pos;
    case Tool::PLACE_BLOCK:
    case Tool::ROTATE_BLOCK:
        cursor.enabled = true;
        break;

    default:
        break;
    }

    // Change selected tool
    tool = newTool;

    // On select
    switch (newTool)
    {
    case Tool::SELECT_BLOCKS:
        cursor.pos = savedPos;
        cursor.type = Game::BlockType::BLOCK;
        break;

    case Tool::PAINT_BLOCK:
        cursor.enabled = false;
        break;
    
    default:
        break;
    }
}

void Editor::UseTool(glm::vec<2, int> mousePos, ActionType actionType)
{
    // Window to eye
    auto winSize = GetWindowSize();
    auto ray = Rendering::ScreenToWorldRay(camera, mousePos, glm::vec2{winSize.x, winSize.y});
    if(cameraController.GetMode() == ::App::Client::CameraMode::FPS)
        ray = Collisions::Ray{camera.GetPos(), camera.GetPos() + camera.GetFront()};

    // Check intersection
    Game::RayBlockIntersection intersect;

#ifdef _DEBUG
    if (gui.optimizeIntersection)
    {
#endif
        intersect = Game::CastRayFirst(project.map.GetMap(), ray, blockScale);
#ifdef _DEBUG
    }
    else
    {
        auto intersections = Game::CastRay(project.map.GetMap(), ray, blockScale);
        auto scale = blockScale;
        std::sort(intersections.begin(), intersections.end(), [ray, scale](const auto& a, const auto& b)
        {
            auto distA = glm::length(glm::vec3{a.pos} * scale - ray.origin);
            auto distB = glm::length(glm::vec3{b.pos} * scale - ray.origin);
            return distA < distB;
        });
        if (!intersections.empty())
            intersect = intersections.front();
    }
#endif

    auto intersection = intersect.intersection;
    intersecting = intersect.intersection.intersects;
    if(intersecting)
        pointedBlockPos = intersect.pos;
    else
        pointedBlockPos = glm::ivec3{0};

    // Use appropiate Tool
    switch(tool)
    {
        case PLACE_BLOCK:
        {
            if(intersect.intersection.intersects)
            {
                auto block = *intersect.block;
                auto blockTransform = Game::GetBlockTransform(block, intersect.pos, blockScale);

                if(actionType == ActionType::LEFT_BUTTON)
                {
                    if(IsDisplayValid())
                    {           
                        auto iNewPos = intersect.pos + glm::ivec3{glm::round(intersection.normal)};
                        if(project.map.CanPlaceBlock(iNewPos))
                        {
                            auto display = GetBlockDisplay(); 
                            auto action = std::make_unique<PlaceBlockAction>(iNewPos, gui.placeBlock, &project.map);

                            DoToolAction(std::move(action));
                        }
                    }
                }
                else if(actionType == ActionType::RIGHT_BUTTON)
                {
                    if(project.map.GetBlockCount() > 1)
                    {
                        auto action = std::make_unique<RemoveAction>(intersect.pos, block, &project.map);
                        DoToolAction(std::move(action));
                    }
                }
                else if(actionType == ActionType::HOVER)
                {
                    auto cursorPos = intersect.pos + glm::ivec3{glm::round(intersection.normal)};
                    if(project.map.CanPlaceBlock(cursorPos))
                        SetCursorState(true, cursorPos, gui.placeBlock.type, gui.placeBlock.rot);
                    else
                        cursor.enabled = false;
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
            if(intersection.intersects)
            {
                auto block = *intersect.block;
                if(actionType == ActionType::LEFT_BUTTON || actionType == ActionType::RIGHT_BUTTON)
                {
                    if(block.type == Game::BlockType::SLOPE)
                    {
                        Game::BlockRot blockRot = GetNextValidRotation(block.rot, gui.axis, actionType == ActionType::LEFT_BUTTON);

                        DoToolAction(std::make_unique<RotateAction>(intersect.pos, &block, &project.map, blockRot));
                    }
                }
                if(actionType == ActionType::HOVER)
                {
                    if(block.type == Game::BlockType::SLOPE)
                    {
                        auto blockTransform = Game::GetBlockTransform(block, intersect.pos, blockScale);
                        SetCursorState(true, intersect.pos, Game::BlockType::BLOCK, cursor.rot);
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
            if(intersection.intersects)
            {
                auto block = *intersect.block;
                auto iPos = intersect.pos;
                if(actionType == ActionType::LEFT_BUTTON || actionType == ActionType::HOLD_LEFT_BUTTON)
                {
                    auto display = GetBlockDisplay();
                    auto block = project.map.GetBlock(iPos);
                    if(IsDisplayValid() && display != block.display)
                        DoToolAction(std::make_unique<PaintAction>(iPos, display, &project.map));
                }
                if(actionType == ActionType::RIGHT_BUTTON)
                {
                    SetBlockDisplay(block.display);
                }
                if(actionType == ActionType::HOVER)
                {
                    pointedBlockPos = iPos;
                }
            }
            break;
        }
        case SELECT_BLOCKS:
        {
            if(intersection.intersects)
            {
                if(actionType == ActionType::LEFT_BUTTON)
                {
                    auto& block = *intersect.block;
                    auto blockTransform = Game::GetBlockTransform(block, intersect.pos, blockScale);
                    cursor.enabled = true;
                    auto offset = movingSelection ? glm::ivec3{intersection.normal} : glm::ivec3{0};
                    auto nextPos = intersect.pos + offset;
                    MoveSelectionCursor(nextPos);
                }
            }
            break;
        }
        case PLACE_OBJECT:
        {
            if(intersect.intersection.intersects && intersection.normal.y == 1.0f)
            {
                auto block = *intersect.block;

                if(actionType == ActionType::LEFT_BUTTON)
                {                          
                    auto iNewPos = intersect.pos + glm::ivec3{0, 1, 0}; // It will be added over the collided block
                    auto iNewPosCeil = iNewPos + glm::ivec3{0, 1, 0}; // We need another block as well
                    if(project.map.CanPlaceBlock(iNewPos) && project.map.CanPlaceBlock(iNewPosCeil))
                    {
                        auto action = std::make_unique<PlaceObjectAction>(placedGo, &project.map, iNewPos);

                        DoToolAction(std::move(action));
                    }
                }
                else if(actionType == ActionType::HOVER)
                {
                    auto cursorPos = intersect.pos + glm::ivec3{glm::round(intersection.normal)};
                    if(project.map.CanPlaceBlock(cursorPos))
                    {
                        SetCursorState(true, cursorPos, gui.placeBlock.type, cursor.rot);
                    }
                    else
                        cursor.enabled = false;
                }
                else if(actionType == ActionType::RIGHT_BUTTON)
                {
                    auto rpos = intersect.pos + glm::ivec3{0, 1, 0};
                    if(auto go = project.map.GetMap()->GetGameObject(rpos))
                    {
                        auto action = std::make_unique<RemoveObjectAction>(&project.map, rpos);
                        DoToolAction(std::move(action));
                    }
                }
            }
            else
            {
                cursor.enabled = false;
            }
            break;
        }
        case SELECT_OBJECT:
        {
            if(intersect.intersection.intersects && intersection.normal.y == 1.0f)
            {
                auto oPos = intersect.pos + glm::ivec3{0, 1, 0};
                if(actionType == ActionType::LEFT_BUTTON || actionType == ActionType::RIGHT_BUTTON)
                {
                    
                    SelectGameObject(oPos);
                }
                else if(actionType == ActionType::HOVER)
                {
                    if(auto go = project.map.GetMap()->GetGameObject(oPos))
                        SetCursorState(true, oPos, Game::BlockType::BLOCK, cursor.rot);
                    else if(auto go = project.map.GetMap()->GetGameObject(gui.selectedObj))
                        SetCursorState(true, gui.selectedObj, Game::BlockType::BLOCK, cursor.rot);
                    else
                        cursor.enabled = false;
                }
            }
            break;
        }
    }
}

void Editor::QueueAction(std::unique_ptr<ToolAction> action)
{
    actionHistory.erase(actionHistory.begin() + actionIndex, actionHistory.end());
    actionHistory.push_back(std::move(action));
}

void Editor::DoToolAction(std::unique_ptr<ToolAction> action)
{
    QueueAction(std::move(action));
    DoToolAction();
}

void Editor::DoToolAction()
{
    if(actionIndex < actionHistory.size())
    {
        auto& action = actionHistory.at(actionIndex);
        action->Do();
        actionIndex++;

        SetUnsaved(true);
    }
    else
        logger->LogDebug("Could not do action");
}

void Editor::UndoToolAction()
{
    if(actionIndex > 0)
    {
        actionIndex--;
        auto& action = actionHistory.at(actionIndex);
        action->Undo();

        if(actionIndex > 0)
            SetUnsaved(true);
        else
            SetUnsaved(false);
    }
    else
        logger->LogDebug("Could not undo anymore");
}

void Editor::ClearActionHistory()
{
    actionIndex = 0;
    actionHistory.clear();
}

// #### Paint Tool #### \\

Game::Display Editor::GetBlockDisplay()
{
    return gui.placeBlock.display;
}

bool Editor::IsDisplayValid()
{
    auto display = GetBlockDisplay();
    auto texCount = project.map.tPalette.GetCount();
    bool isValidTexture = display.type == Game::DisplayType::TEXTURE && display.id < texCount;
    bool isColor = display.type == Game::DisplayType::COLOR;
    return isColor || isValidTexture;
}

void Editor::SetBlockDisplay(Game::Display display)
{
    gui.placeBlock.display = display;
}

// #### Selection Cursor #### \\

void Editor::DrawCursor(Math::Transform t)
{
    // Draw cursor
    auto model = t.GetTransformMat();
    auto transform = camera.GetProjViewMat() * model;
    shader.SetUniformMat4("transform", transform);
    shader.SetUniformInt("hasBorder", false);
    shader.SetUniformInt("overrideColor", true);
    shader.SetUniformInt("textureType", 1);
    shader.SetUniformVec4("color", cursor.color);
    auto& mesh = GetMesh(cursor.type);
    //mesh.Draw(shader, cursor.color, GL_LINE);
    glDisable(GL_CULL_FACE);
    mesh.Draw(shader, GL_LINE);
    glEnable(GL_CULL_FACE);
}

void Editor::DrawSelectCursor(glm::ivec3 pos)
{
    if(cursor.mode == CursorMode::BLOCKS)
    {
        for(int x = 0; x < cursor.scale.x; x++)
        {
            for(int y = 0; y < cursor.scale.y; y++)
            {
                for(int z = 0; z < cursor.scale.z; z++)
                {
                    auto ipos = pos + glm::ivec3{x, y, z};
                    auto t = Game::GetBlockTransform(ipos, blockScale);
                    
                    DrawCursor(t);
                }
            }
        }
    }
    else if(cursor.mode == CursorMode::SCALED) 
    {
        auto scale = (glm::vec3)cursor.scale * blockScale;
        auto tPos = Game::Map::ToRealPos(pos, blockScale) + (scale / 2.0f) - glm::vec3{blockScale / 2};
        Math::Transform t{tPos, glm::vec3{0.0f}, scale};

        DrawCursor(t);
    }
}

void Editor::SetCursorState(bool enabled, glm::ivec3 pos, Game::BlockType blockType, Game::BlockRot blockRot)
{
    cursor.enabled = enabled;
    cursor.pos = pos;
    cursor.type = blockType;
    cursor.rot = blockRot;
}

void Editor::MoveSelectionCursor(glm::ivec3 nextPos)
{
    if(movingSelection)
    {
        glm::ivec3 offset = nextPos - cursor.pos;
        if(CanMoveSelection(offset))
            MoveSelection(offset);
    }
    else
        cursor.pos = nextPos;
}

// #### Select Tool #### \\

void Editor::OnChooseSelectSubTool(SelectSubTool subTool)
{
    // Always stop moving selection
    movingSelection = false;
    ClearSelection();
}

void Editor::EnumBlocksInSelection(std::function<void(glm::ivec3, glm::ivec3)> onEach)
{
    auto scale = glm::vec3{blockScale};
    glm::ivec3 cursorBasePos = cursor.pos;
    for(unsigned int x = 0; x < cursor.scale.x; x++)
    {
        for(unsigned int y = 0; y < cursor.scale.y; y++)
        {
            for(unsigned int z = 0; z < cursor.scale.z; z++)
            {
                auto offset = glm::ivec3{x, y, z};
                auto ipos = cursorBasePos + offset;
                onEach(ipos, offset);
            }
        }
    }
}

std::vector<BlockData> Editor::GetBlocksInSelection(bool globalPos)
{
    std::vector<BlockData> selection;

    auto scale = glm::vec3{blockScale};
    glm::ivec3 cursorBasePos = cursor.pos;
    auto onEach = [this, &selection, globalPos](glm::ivec3 ipos, glm::ivec3 offset)
    {
        if(!this->project.map.IsNullBlock(ipos))
        {
            if(globalPos)
                selection.push_back({ipos, this->project.map.GetBlock(ipos)});
            else
                selection.push_back({offset, this->project.map.GetBlock(ipos)});
        }
    };
    EnumBlocksInSelection(onEach);

    return selection;
}

void Editor::SelectBlocks()
{
    selection = GetBlocksInSelection();

   logger->LogDebug("Selected " + std::to_string(selection.size()) + " blocks");
}

void Editor::ClearSelection()
{
    selection.clear();
}

bool Editor::CanMoveSelection(glm::ivec3 offset)
{   
    for(const auto& pair : selection)
    {
        auto pos = pair.first + offset;
        if(!project.map.IsNullBlock(pos) && !IsBlockInSelection(pos))
            return false;
    }

    return true;

}

bool Editor::IsBlockInSelection(glm::ivec3 pos)
{
    for(const auto& pair : selection)
        if(pair.first == pos)
            return true;

    return false;
}

void Editor::MoveSelection(glm::ivec3 offset)
{
    DoToolAction(std::make_unique<MoveSelectionAction>(&project.map, selection, offset, &cursor.pos, &selection));
}

// #### Select Tool - Edit #### \\

void Editor::CopySelection()
{
    clipboard = GetBlocksInSelection(false);

    logger->LogDebug(std::to_string(clipboard.size()) + std::string(" copied to clipboard"));
}

void Editor::RemoveSelection()
{
    auto blockData = GetBlocksInSelection();
    auto batchRemove = std::make_unique<BatchedAction>();
    for(auto& bData : blockData)
    {
        auto removeAction = std::make_unique<RemoveAction>(bData.first, bData.second, &project.map);
        batchRemove->AddAction(std::move(removeAction));
    }
    DoToolAction(std::move(batchRemove));
}

void Editor::CutSelection()
{
    CopySelection();
    RemoveSelection();
}

void Editor::PasteSelection()
{
    auto batchPlace = std::make_unique<BatchedAction>();
    for(auto& bData : clipboard)
    {
        auto pos = cursor.pos + bData.first;
        if(project.map.IsNullBlock(pos))
        {
            auto placeAction = std::make_unique<PlaceBlockAction>(pos, bData.second, &project.map);
            batchPlace->AddAction(std::move(placeAction));
        }
    }
    DoToolAction(std::move(batchPlace));
}

Editor::Result Editor::RotateSelection(Game::RotationAxis axis, Game::RotType rotType)
{
    Result res;

    //Check cursor is a square in the involved axis;
    if(rotType == Game::RotType::ROT_90)
    {
        bool canRotateX = axis ==Game::RotationAxis::X && cursor.scale.y == cursor.scale.z;
        bool canRotateY = axis ==Game::RotationAxis::Y && cursor.scale.x == cursor.scale.z;
        bool canRotateZ = axis ==Game::RotationAxis::Z && cursor.scale.x == cursor.scale.y;
        bool canRotate = canRotateX || canRotateY || canRotateZ;

        if(!canRotate)
        {
            res.isOk = false;
            res.info = "Can not rotate 90 deg with a non-squared selection";
            return res;
        }
    }

    auto lselection = GetBlocksInSelection();
    float angle = rotType == Game::RotType::ROT_90 ? 90.0f : 180.0f;

    std::vector<std::pair<glm::vec3, Game::Block>> rotSelection;
    rotSelection.reserve(lselection.size());

    // Calculate rot matrix
    glm::ivec3 rotAxis = glm::vec3{0, 1, 0};
    if(axis == Game::RotationAxis::X)
        rotAxis = glm::vec3{1, 0, 0};
    else if(axis == Game::RotationAxis::Z)
        rotAxis = glm::vec3{0, 0, 1};
    glm::mat3 rotMat = glm::rotate(glm::mat4{1}, glm::radians(angle), glm::vec3{rotAxis});

    // Convert to square mat
    glm::vec3 adjustedOffset = glm::vec3(cursor.scale - 1) / 2.0f;
    glm::vec3 center = glm::vec3{cursor.pos} + adjustedOffset;

    // Batched action
    auto batch = std::make_unique<BatchedAction>();

    for(auto bData : lselection)
    {
        glm::vec3 offset = glm::vec3{bData.first} - center;
        glm::vec3 rotOffset = rotMat * offset;
        rotSelection.push_back({rotOffset, bData.second});

        batch->AddAction(std::make_unique<RemoveAction>(bData.first, bData.second, &project.map));
    }

    for(auto bData : rotSelection)
    {
        // Rotate slope
        if(bData.second.type == Game::BlockType::SLOPE)
        {
            auto bRot = bData.second.rot;
            if(axis ==Game::RotationAxis::Y)
            {
                bData.second.rot = GetNextValidRotation(bData.second.rot, axis, true);
                if(rotType == Game::RotType::ROT_180)
                    bData.second.rot = GetNextValidRotation(bData.second.rot, axis, true);
            }
            else if(axis ==Game::RotationAxis::Z)
            {
                if(rotType == Game::RotType::ROT_90)
                {
                    if(bRot.y == Game::RotType::ROT_0)
                    {
                        bData.second.rot = GetNextValidRotation(bData.second.rot, axis, true);
                    }
                    else if(bRot.y == Game::RotType::ROT_90)
                    {
                        if(bRot.z == Game::RotType::ROT_180)
                        {
                            bData.second.rot = GetNextValidRotation(bData.second.rot, axis, true);
                            bData.second.rot = GetNextValidRotation(bData.second.rot, axis, true);
                        }
                        else if(bRot.z == Game::RotType::ROT_90)
                        {
                            bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Y, false);
                            bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Z, false);
                        }
                        else if(bRot.z == Game::RotType::ROT_270)
                        {
                            bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Z, true);
                            bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Y, true);
                        }   
                        else
                        {
                            bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Y, true);
                            bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Y, true);
                        }
                    }
                    else if(bRot.y == Game::RotType::ROT_180)
                    {
                        bData.second.rot = GetNextValidRotation(bData.second.rot, axis, false);
                    }
                    else if(bRot.y == Game::RotType::ROT_270)
                    {
                        if(bRot.z == Game::RotType::ROT_180)
                        {
                            bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Y, true);
                            bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Y, true);
                        }
                        else if(bRot.z == Game::RotType::ROT_90)
                        {
                            bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Y, false);
                            bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Z, true);
                        }
                        else if(bRot.z == Game::RotType::ROT_270)
                        {
                            bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Y, true);
                            bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Z, false);
                        }   
                        else
                        {
                            bData.second.rot = GetNextValidRotation(bData.second.rot, axis, true);
                            bData.second.rot = GetNextValidRotation(bData.second.rot, axis, true);
                        }
                    }
                }
                else if(rotType == Game::RotType::ROT_180)
                {
                    bData.second.rot = GetNextValidRotation(bData.second.rot, axis, true);
                    bData.second.rot = GetNextValidRotation(bData.second.rot, axis, true);

                    if(bRot.y == Game::RotType::ROT_90 || bRot.y == Game::ROT_270)
                    {
                        bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Y, true);
                        bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Y, true);
                    }
                }
            }
            else if(axis ==Game::RotationAxis::X)
            {
                if(rotType == Game::RotType::ROT_90)
                {
                    if(bRot.y == Game::RotType::ROT_0)
                    {
                        if(bRot.z == Game::RotType::ROT_90)
                        {
                            bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Y, false);
                            bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Z, true);
                        }
                        else if(bRot.z == Game::RotType::ROT_180)
                        {
                            bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Y, true);
                            bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Y, true);    
                        }
                        else if(bRot.z == Game::RotType::ROT_270)
                        {
                            bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Y, true);
                            bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Z, false);
                        }
                        else
                        {
                            bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Z, true);
                            bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Z, true);
                        }
                    }
                    else if(bRot.y == Game::RotType::ROT_90)
                    {
                        bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Z, true);
                    }
                    else if(bRot.y == Game::RotType::ROT_180)
                    {
                        if(bRot.z == Game::RotType::ROT_90)
                        {
                            bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Y, false);
                            bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Z, false);
                        }
                        else if(bRot.z == Game::RotType::ROT_180)
                        {
                            bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Z, true);
                            bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Z, true);    
                        }
                        else if(bRot.z == Game::RotType::ROT_270)
                        {
                            bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Y, true);
                            bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Z, true);
                        }
                        else
                        {
                            bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Y, true);
                            bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Y, true);
                        }
                    }
                    else if(bRot.y == Game::RotType::ROT_270)
                    {
                        bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Z, false);
                    }
                }
                else if(rotType == Game::RotType::ROT_180)
                {
                    bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Z, true);
                    bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Z, true);

                    if(bRot.y == Game::RotType::ROT_0 || bRot.y == Game::ROT_180)
                    {
                        bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Y, true);
                        bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Y, true);
                    }
                }
            }
        }

        glm::ivec3 absPos = glm::round(center + bData.first);
        batch->AddAction(std::make_unique<PlaceBlockAction>(absPos, bData.second, &project.map));
    }

    if(rotType == Game::ROT_90)
    {
        // Rotate scale
        auto cs = cursor.scale;
        cursor.scale = axis ==Game::RotationAxis::Y ? glm::ivec3{cs.z, cs.y, cs.x} : glm::ivec3{cs.y, cs.x, cs.z};
    }

    DoToolAction(std::move(batch));

    return res;
}

static glm::ivec3 GetRefBlock(MirrorPlane plane, glm::ivec3 blockPos, glm::ivec3 cursorPos, glm::ivec3 cursorScale)
{
    glm::ivec3 refBlock;
    switch (plane)
    {
    case MirrorPlane::XY:
        refBlock = glm::ivec3{blockPos.x, blockPos.y, cursorPos.z - 1};
        break;
    
    case MirrorPlane::XZ:
        refBlock = glm::ivec3{blockPos.x, cursorPos.y - 1, blockPos.z};
        break;
    
    case MirrorPlane::YZ:
        refBlock = glm::ivec3{cursorPos.x - 1, blockPos.y, blockPos.z};
        break;

    case MirrorPlane::NOT_XY:
        refBlock = glm::ivec3{blockPos.x, blockPos.y, cursorPos.z + cursorScale.z};
        break;
    
    case MirrorPlane::NOT_XZ:
        refBlock = glm::ivec3{blockPos.x, cursorPos.y + cursorScale.y, blockPos.z};
        break;
    
    case MirrorPlane::NOT_YZ:
        refBlock = glm::ivec3{cursorPos.x + cursorScale.x, blockPos.y , blockPos.z};
        break;
    
    default:
        break;
    }

    return refBlock;
}

Editor::Result Editor::MirrorSelection(MirrorPlane plane)
{
    Result res;

    auto lselection = GetBlocksInSelection();
    auto batch = std::make_unique<BatchedAction>();

    for(auto bData : lselection)
    {
        glm::ivec3 refBlock = GetRefBlock(plane, bData.first, cursor.pos, cursor.scale);
        glm::ivec3 offset = bData.first - refBlock;
        glm::ivec3 mirrorPos = refBlock - offset;
        
        if(bData.second.type == Game::BlockType::SLOPE)
        {
            auto bRot = bData.second.rot;

            if(plane == MirrorPlane::XY || plane == MirrorPlane::NOT_XY)
            {
                if(bRot.z == Game::RotType::ROT_0 || bRot.z == Game::RotType::ROT_180)
                {
                    if(bRot.y == Game::RotType::ROT_0 || bRot.y == Game::RotType::ROT_180)
                    {
                        bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Y, true);
                        bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Y, true);
                    }
                }
                else if(bRot.z == Game::RotType::ROT_270 || bRot.z == Game::RotType::ROT_90)
                {
                    bool sign = bRot.z == Game::RotType::ROT_90;
                    if(bRot.y == Game::RotType::ROT_0)
                        bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Y, !sign);
                    else if(bRot.y == Game::RotType::ROT_90)
                        bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Y, sign);
                    else if(bRot.y == Game::RotType::ROT_180)
                        bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Y, !sign);
                    else if(bRot.y == Game::RotType::ROT_270)
                        bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Y, sign);
                }
            }
            else if(plane == MirrorPlane::XZ || plane == MirrorPlane::NOT_XZ)
            {
                if(bRot.z == Game::RotType::ROT_0 || bRot.z == Game::RotType::ROT_180)
                {
                    bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Z, true);
                    bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Z, true);
                }
            }
            else if(plane == MirrorPlane::YZ || plane == MirrorPlane::NOT_YZ)
            {
                if(bRot.z == Game::RotType::ROT_0 || bRot.z == Game::RotType::ROT_180)
                {
                    if(bRot.y == Game::RotType::ROT_90 || bRot.y == Game::RotType::ROT_270)
                    {
                        bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Y, true);
                        bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Y, true);
                    }
                }
                else if(bRot.z == Game::RotType::ROT_90 || bRot.z == Game::RotType::ROT_270)
                {
                    bool sign = bRot.z == Game::RotType::ROT_90;
                    if(bRot.y == Game::RotType::ROT_0)
                        bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Y, sign);
                    else if(bRot.y == Game::RotType::ROT_90)
                        bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Y, !sign);
                    else if(bRot.y == Game::RotType::ROT_180)
                        bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Y, sign);
                    else if(bRot.y == Game::RotType::ROT_270)
                        bData.second.rot = GetNextValidRotation(bData.second.rot,Game::RotationAxis::Y, !sign);
                }
            }

        }
        if(project.map.IsNullBlock(mirrorPos))
            batch->AddAction(std::make_unique<PlaceBlockAction>(mirrorPos, bData.second, &project.map));
        else
            batch->AddAction(std::make_unique<UpdateBlockAction>(mirrorPos, bData.second, &project.map));
    }

    DoToolAction(std::move(batch));

    return res;
}

void Editor::ReplaceAllInSelection()
{
    auto batchPlace = std::make_unique<BatchedAction>();
    auto onEach = [this, &batchPlace](glm::ivec3 pos, glm::ivec3 offset)
    {
        if(!this->project.map.IsNullBlock(pos))
        {
            auto removeAction = std::make_unique<RemoveAction>(pos, this->project.map.GetBlock(pos), &this->project.map);
            batchPlace->AddAction(std::move(removeAction));
        }
        auto placeAction = std::make_unique<PlaceBlockAction>(pos, gui.placeBlock, &this->project.map);
        batchPlace->AddAction(std::move(placeAction));
    };
    EnumBlocksInSelection(onEach);
    DoToolAction(std::move(batchPlace));
}

void Editor::FillEmptyInSelection()
{
    auto batchPlace = std::make_unique<BatchedAction>();
    auto onEach = [this, &batchPlace](glm::ivec3 pos, glm::ivec3 offset)
    {
        if(this->project.map.IsNullBlock(pos))
        {
    
            auto placeAction = std::make_unique<PlaceBlockAction>(pos, gui.placeBlock, &this->project.map);
            batchPlace->AddAction(std::move(placeAction));
        }
    };
    EnumBlocksInSelection(onEach);
    DoToolAction(std::move(batchPlace));
}

void Editor::ReplaceAnyInSelection()
{
    auto batchPlace = std::make_unique<BatchedAction>();
    auto onEach = [this, &batchPlace](glm::ivec3 pos, glm::ivec3 offset)
    {
        if(!this->project.map.IsNullBlock(pos))
        {
            auto placeAction = std::make_unique<UpdateBlockAction>(pos, gui.placeBlock, &project.map);
            batchPlace->AddAction(std::move(placeAction));
        }
    };
    EnumBlocksInSelection(onEach);
    DoToolAction(std::move(batchPlace));
}

void Editor::ReplaceInSelection()
{
    auto batchPlace = std::make_unique<BatchedAction>();
    auto onEach = [this, &batchPlace](glm::ivec3 pos, glm::ivec3 offset)
    {
        auto block = this->project.map.GetBlock(pos);
        if(block == this->gui.findBlock)
        {
            auto source = gui.placeBlock;
            source.rot = block.rot;
            
            auto placeAction = std::make_unique<UpdateBlockAction>(pos, source, &project.map);
            batchPlace->AddAction(std::move(placeAction));
        }
    };
    EnumBlocksInSelection(onEach);
    DoToolAction(std::move(batchPlace));
}

void Editor::PaintSelection()
{
    auto batchPlace = std::make_unique<BatchedAction>();
    auto onEach = [this, &batchPlace](glm::ivec3 pos, glm::ivec3 offset)
    {
        if(!this->project.map.IsNullBlock(pos))
        {
            auto block = this->project.map.GetBlock(pos);
            auto display = this->GetBlockDisplay();
            auto placeAction = std::make_unique<PaintAction>(pos, display, &project.map);
            batchPlace->AddAction(std::move(placeAction));
        }
    };
    EnumBlocksInSelection(onEach);
    DoToolAction(std::move(batchPlace));
}

void Editor::ReplaceAll(Game::Block source, Game::Block target)
{
    auto batchPlace = std::make_unique<BatchedAction>();

    auto it = this->project.map.GetMap()->CreateIterator();
    for(auto [pos, block] = it.GetNextBlock(); !it.IsOver(); std::tie(pos, block) = it.GetNextBlock())
    {
        if(block && *block == target)
        {
            auto removeAction = std::make_unique<RemoveAction>(pos, *block, &this->project.map);
            batchPlace->AddAction(std::move(removeAction));

            source.rot = block->rot;
            auto placeAction = std::make_unique<PlaceBlockAction>(pos, source, &this->project.map);
            batchPlace->AddAction(std::move(placeAction));
        }
    }
    DoToolAction(std::move(batchPlace));
}

// #### Object Tools #### \\

void Editor::Editor::SelectGameObject(glm::ivec3 pos)
{
    if(auto object = project.map.GetMap()->GetGameObject(pos))
    {
        // Copy data to placedGo
        placedGo = *object;
        gui.selectedObj = pos;
    }
}

void Editor::EditGameObject()
{
    auto place = std::make_unique<PlaceObjectAction>(placedGo, &project.map, gui.selectedObj);
    auto remove = std::make_unique<RemoveObjectAction>(&project.map, gui.selectedObj);
    auto batch = std::make_unique<BatchedAction>();
    batch->AddAction(std::move(remove));
    batch->AddAction(std::move(place));
    DoToolAction(std::move(batch));
}

// #### Options #### \\

void Editor::ApplyVideoOptions(::App::Configuration::WindowConfig& winConfig)
{
    AppI::ApplyVideoOptions(winConfig);

    auto winSize = GetWindowSize();
    camera.SetParam(camera.ASPECT_RATIO, (float) winSize.x/ (float) winSize.y);
    camera.SetParam(Rendering::Camera::Param::FOV, winConfig.fov);
}
