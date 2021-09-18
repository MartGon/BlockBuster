#include <Editor.h>

#include <game/Game.h>
#include <math/BBMath.h>

#include <iostream>
#include <algorithm>
#include <fstream>
#include <cstring>

#include <glm/gtc/constants.hpp>

#include <debug/Debug.h>

#include <imgui/backends/imgui_impl_opengl3.h>

// #### Public Interface #### \\

void BlockBuster::Editor::Editor::Start()
{
    // Shaders
    shader.Use();

    // Meshes
    cube = Rendering::Primitive::GenerateCube();
    slope = Rendering::Primitive::GenerateSlope();
    
    // OpenGL features
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // Camera
    int width, height;
    SDL_GetWindowSize(window_, &width, &height);
    camera.SetParam(Rendering::Camera::Param::ASPECT_RATIO, (float)width / (float)height);
    camera.SetParam(Rendering::Camera::Param::FOV, config.window.fov);
    
    // World
    mapsFolder = GetConfigOption("MapsFolder", ".");
    auto mapName = GetConfigOption("Map", "Map.bbm");
    bool mapLoaded = false;
    if(!mapName.empty() && mapName.size() < 16)
    {
        std::strcpy(fileName, mapName.c_str());
        mapLoaded = OpenProject().type == General::ResultType::SUCCESS;
    }
    
    if(!mapLoaded)
        NewProject();

    // Cursor
    cursor.show = GetConfigOption("showCursor", "1") == "1";

    // GUI
    InitPopUps();
}

void BlockBuster::Editor::Editor::Update()
{
    chunkMeshMgr.Update();

    // Clear Buffer
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Setup texture arrays
    project.tPalette.GetTextureArray()->Bind(GL_TEXTURE0);
    chunkShader.SetUniformInt("textureArray", 0);
    project.cPalette.GetTextureArray()->Bind(GL_TEXTURE1);
    chunkShader.SetUniformInt("colorArray", 1);

    // Draw pre view painted block
    bool isPainted = intersecting && tool == PAINT_BLOCK;
    if(isPainted)
    {
        auto block = project.map.GetBlock(pointedBlockPos);
        Math::Transform t = Game::GetBlockTransform(*block, pointedBlockPos, blockScale);

        // HACK: To avoid z fighting.
        // Option 2: Changing block display each time the block is pointed/unpointd. Create wrapper function to do it safely. 
        //  Revert changes when saving project
        const auto factor = 1.025f;
        t.scale *= factor;

        if(block->type == Game::BlockType::SLOPE)
        {
            const glm::vec3 up{0.0f, 1.0f, 0.0f};
            glm::vec3 rot = t.GetRotationMat() * glm::vec4{up, 1.0f};
            auto offset = (factor - 1.f) / 2.0f;
            t.position += (rot * offset);
        }
        
        auto mMat = t.GetTransformMat();
        auto tMat = camera.GetProjViewMat() * mMat;
        auto& mesh = GetMesh(block->type);

        auto display =  GetBlockDisplay();
    
        shader.SetUniformMat4("transform", tMat);
        shader.SetUniformInt("textureType", display.type);
        shader.SetUniformInt("textureId", display.id);
        shader.SetUniformInt("overrideColor", false);

        shader.SetUniformInt("textureArray", 0);
        shader.SetUniformInt("colorArray", 1);

        mesh.Draw(shader, project.tPalette.GetTextureArray(), display.id);
    }

    // Draw new Map System Cubes
    chunkMeshMgr.DrawChunks(chunkShader, camera.GetProjViewMat());
    
    if(playerMode)
        UpdatePlayerMode();
    else
        UpdateEditor();

    SDL_GL_SwapWindow(window_);
}

bool BlockBuster::Editor::Editor::Quit()
{
    return quit;
}

void BlockBuster::Editor::Editor::Shutdown()
{
    config.options["Map"] = std::string(fileName);
    config.options["TextureFolder"] = textureFolder.string();
    config.options["MapsFolder"] = mapsFolder.string();
    config.options["showCursor"] = std::to_string(cursor.show);
}

// #### Rendering #### \\

Rendering::Mesh& BlockBuster::Editor::Editor::GetMesh(Game::BlockType blockType)
{
    return blockType == Game::BlockType::SLOPE ? slope : cube;
}

glm::vec4 BlockBuster::Editor::Editor::GetBorderColor(glm::vec4 basecolor, glm::vec4 darkColor, glm::vec4 lightColor)
{
    auto color = basecolor;
    auto darkness = (color.r + color.g + color.b) / 3.0f;
    auto borderColor = darkness <= 0.5 ? lightColor : darkColor;
    return borderColor;
}

General::Result<bool> BlockBuster::Editor::Editor::LoadTexture()
{
    if(project.tPalette.GetCount() >= MAX_TEXTURES)
        return General::CreateError<bool>("Maximum of textures reached");

    if(IsTextureInPalette(textureFolder, textureFilename))
        return General::CreateError<bool>("Texture is already in palette");

    auto res = project.tPalette.AddTexture(textureFolder, textureFilename, false);
    if(res.type == General::ResultType::ERROR)
    {
        return General::CreateError<bool>(res.err.info);
    }
    else
        SyncGUITextures();

    return General::CreateSuccess<bool>(true);
}

bool BlockBuster::Editor::Editor::IsTextureInPalette(std::filesystem::path folder, std::filesystem::path textureName)
{
    auto texturePath = folder / textureName;
    for(int i = 0; i < project.tPalette.GetCount(); i++)
        if(project.tPalette.GetMember(i).data.filepath == texturePath)
            return true;
    
    return false;
}

// #### World #### \\

void BlockBuster::Editor::Editor::NewProject()
{
    // Init project
    project = Project{};
    project.Init();
    SyncGUITextures();

    // Camera
    camera.SetPos(glm::vec3 {0.0f, 6.0f, 6.0f});
    camera.SetTarget(glm::vec3{0.0f});

    // Window
    RenameMainWindow("New Map");

    // Flags
    newMap = true;
    unsaved = false;

    // Filename
    std::strcpy(fileName, "NewMap.bbm");

    ClearActionHistory();

    // Chunk meshes
    chunkMeshMgr.SetMap(&project.map);
    chunkMeshMgr.SetBlockScale(blockScale);
}

void BlockBuster::Editor::Editor::SaveProject()
{
    std::filesystem::path mapPath = mapsFolder / fileName;

    // Write project file
    // Camera
    project.cameraPos = camera.GetPos();
    project.cameraRot = camera.GetRotation();

    // Texture folder
    project.textureFolder = textureFolder;

    // Cursor Pos
    project.cursorPos = cursor.pos;
    project.cursorScale = cursor.scale;
    ::BlockBuster::Editor::WriteProjectToFile(project, mapPath);

    RenameMainWindow(fileName);

    // Update flag
    newMap = false;
    unsaved = false;
}

General::Result<bool> BlockBuster::Editor::Editor::OpenProject()
{
    std::filesystem::path mapPath = mapsFolder / fileName;
    Project temp = ::BlockBuster::Editor::ReadProjectFromFile(mapPath);

    auto res = General::CreateError<bool>("Could not open project");

    bool isOk = temp.isOk;
    if(isOk)
    {
        // Move to main project if it's ok
        project = std::move(temp);

        // Window
        RenameMainWindow(fileName);

        // Update flag
        newMap = false;
        unsaved = false;

        // Color Palette
        colorPick = Rendering::Uint8ColorToFloat(project.cPalette.GetMember(0).data.color);

        // Clear history
        ClearActionHistory();

        // Set camera pos
        camera.SetPos(project.cameraPos);
        camera.SetRotation(project.cameraRot.x, project.cameraRot.y);

        // Set cursor
        cursor.pos = project.cursorPos;
        savedPos = project.cursorPos;
        cursor.scale = project.cursorScale;

        SyncGUITextures();

        // Chunk meshes
        chunkMeshMgr.SetMap(&project.map);
        chunkMeshMgr.SetBlockScale(blockScale);

        res = General::CreateSuccess<bool>(true);
    }
    
    return res;
}

// #### Editor #### \\

void BlockBuster::Editor::Editor::UpdateEditor()
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
        auto mousePos = GetMousePos();
        UseTool(mousePos, ActionType::HOVER);
    }
    
    // Camera
    if(cameraMode == CameraMode::EDITOR)
        UpdateEditorCamera();
    else if(cameraMode == CameraMode::FPS)
        UpdateFPSCameraPosition();

    // Draw Cursor
    glDisable(GL_DEPTH_TEST);
    if(cursor.enabled && cursor.show && tool != SELECT_BLOCKS)
    {   
        auto t = Game::GetBlockTransform(cursor.pos, cursor.rot, blockScale);
        DrawCursor(t);
    }
    else if(tool == SELECT_BLOCKS)
    {
        DrawSelectCursor(cursor.pos);
    }

    glEnable(GL_DEPTH_TEST);

    // Draw chunk borders
    if(drawChunkBorders)
    {
        auto chunkIt = project.map.CreateChunkIterator();
        for(auto chunk = chunkIt.GetNextChunk(); !chunkIt.IsOver(); chunk = chunkIt.GetNextChunk())
        {
            auto pos = Game::Map::ToRealChunkPos(chunk.first, blockScale);
            auto size = glm::vec3{Game::Map::Map::Chunk::DIMENSIONS} * blockScale;
            Math::Transform ct{pos, glm::vec3{0.0f}, size};
            auto model = ct.GetTransformMat();

            auto transform = camera.GetProjViewMat() * model;
            shader.SetUniformMat4("transform", transform);
            shader.SetUniformInt("hasBorder", false);
            shader.SetUniformInt("overrideColor", true);
            shader.SetUniformInt("textureType", 1);
            shader.SetUniformVec4("color", yellow);
            auto& mesh = GetMesh(cursor.type);
            glDisable(GL_CULL_FACE);
            mesh.Draw(shader, GL_LINE);
            glEnable(GL_CULL_FACE);
        }
    }

    // Create GUI
    GUI();
}

void BlockBuster::Editor::Editor::UpdateEditorCamera()
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
    cameraRot.y = Math::OverflowSumFloat(cameraRot.y, yaw, 0.0f, glm::two_pi<float>());
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

void BlockBuster::Editor::Editor::UpdateFPSCameraPosition()
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

void BlockBuster::Editor::Editor::UpdateFPSCameraRotation(SDL_MouseMotionEvent motion)
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

void BlockBuster::Editor::Editor::SetCameraMode(CameraMode mode)
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

void BlockBuster::Editor::Editor::SelectTool(Tool newTool)
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

void BlockBuster::Editor::Editor::UseTool(glm::vec<2, int> mousePos, ActionType actionType)
{
    // Window to eye
    auto winSize = GetWindowSize();
    auto ray = Rendering::ScreenToWorldRay(camera, mousePos, glm::vec2{winSize.x, winSize.y});
    if(cameraMode == CameraMode::FPS)
        ray = Collisions::Ray{camera.GetPos(), camera.GetPos() + camera.GetFront()};

    // Check intersection
    Game::RayBlockIntersection intersect;

#ifdef _DEBUG
    if (optimizeIntersection)
    {
#endif
        intersect = Game::CastRayFirst(&project.map, ray, blockScale);
#ifdef _DEBUG
    }
    else
    {
        auto intersections = Game::CastRay(&project.map, ray, blockScale);
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

                if(actionType == ActionType::LEFT_BUTTON || actionType == ActionType::HOVER)
                {
                    auto blockTransform = Game::GetBlockTransform(block, intersect.pos, blockScale);
                    Game::BlockRot rot{Game::RotType::ROT_0, Game::RotType::ROT_0};
                    if(blockType == Game::BlockType::SLOPE)
                        rot.y = static_cast<Game::RotType>(glm::round(camera.GetRotation().y / glm::half_pi<float>()) - 1);

                    if(actionType == ActionType::LEFT_BUTTON)
                    {
                        if(IsDisplayValid())
                        {
                                                       
                            auto iNewPos = intersect.pos + glm::ivec3{glm::round(intersection.normal)};
                            if(auto found = project.map.GetBlock(iNewPos); !found || found->type == Game::BlockType::NONE)
                            {
                                auto display = GetBlockDisplay(); 
                                auto block = Game::Block{blockType, rot, display};
                                auto action = std::make_unique<PlaceBlockAction>(iNewPos, block, &project.map);

                                DoToolAction(std::move(action));
                            }
                        }
                    }
                    else if(actionType == ActionType::HOVER)
                    {
                        cursor.pos = intersect.pos + glm::ivec3{glm::round(intersection.normal)};
                        if(auto found = project.map.GetBlock(cursor.pos); !found || found->type == Game::BlockType::NONE)
                        {
                            cursor.enabled = true;
                            cursor.type = blockType;
                            cursor.rot = rot;
                        }
                        else
                            cursor.enabled = false;
                    }
                }
                else if(actionType == ActionType::RIGHT_BUTTON)
                {
                    if(project.map.GetBlockCount() > 1)
                    {
                        auto action = std::make_unique<RemoveAction>(intersect.pos, block, &project.map);
                        DoToolAction(std::move(action));
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
            if(intersection.intersects)
            {
                auto block = *intersect.block;
                if(actionType == ActionType::LEFT_BUTTON || actionType == ActionType::RIGHT_BUTTON)
                {
                    if(block.type == Game::BlockType::SLOPE)
                    {
                        Game::BlockRot blockRot = GetNextValidRotation(block.rot, axis, actionType == ActionType::LEFT_BUTTON);

                        DoToolAction(std::make_unique<RotateAction>(intersect.pos, &block, &project.map, blockRot));
                    }
                }
                if(actionType == ActionType::HOVER)
                {
                    if(block.type == Game::BlockType::SLOPE)
                    {
                        auto blockTransform = Game::GetBlockTransform(block, intersect.pos, blockScale);

                        cursor.enabled = true;
                        cursor.pos = intersect.pos;
                        cursor.type = Game::BlockType::BLOCK;
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
                if(actionType == ActionType::LEFT_BUTTON)
                {
                    auto display = GetBlockDisplay();
                    if(IsDisplayValid())
                        DoToolAction(std::make_unique<PaintAction>(iPos, &block, display, &project.map));
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
    }
}

void BlockBuster::Editor::Editor::QueueAction(std::unique_ptr<ToolAction> action)
{
    actionHistory.erase(actionHistory.begin() + actionIndex, actionHistory.end());
    actionHistory.push_back(std::move(action));
}

void BlockBuster::Editor::Editor::DoToolAction(std::unique_ptr<ToolAction> action)
{
    QueueAction(std::move(action));
    DoToolAction();
}

void BlockBuster::Editor::Editor::DoToolAction()
{
    if(actionIndex < actionHistory.size())
    {
        auto& action = actionHistory.at(actionIndex);
        action->Do();
        actionIndex++;

        SetUnsaved(true);
    }
    else
        std::cout << "Could not do action\n";
}

void BlockBuster::Editor::Editor::UndoToolAction()
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
        std::cout << "Could not undo anymore\n";
}

void BlockBuster::Editor::Editor::ClearActionHistory()
{
    actionIndex = 0;
    actionHistory.clear();
}

void BlockBuster::Editor::Editor::DrawCursor(Math::Transform t)
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

void BlockBuster::Editor::Editor::DrawSelectCursor(glm::ivec3 pos)
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

void BlockBuster::Editor::Editor::EnumBlocksInSelection(std::function<void(glm::ivec3, glm::ivec3)> onEach)
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

std::vector<BlockBuster::Editor::BlockData> BlockBuster::Editor::Editor::GetBlocksInSelection(bool globalPos)
{
    std::vector<BlockBuster::Editor::BlockData> selection;

    auto scale = glm::vec3{blockScale};
    glm::ivec3 cursorBasePos = cursor.pos;
    auto onEach = [this, &selection, globalPos](glm::ivec3 ipos, glm::ivec3 offset)
    {
        if(!this->project.map.IsBlockNull(ipos))
        {
            if(globalPos)
                selection.push_back({ipos, *this->project.map.GetBlock(ipos)});
            else
                selection.push_back({offset, *this->project.map.GetBlock(ipos)});
        }
    };
    EnumBlocksInSelection(onEach);

    return selection;
}

void BlockBuster::Editor::Editor::SelectBlocks()
{
    selection = GetBlocksInSelection();

    std::cout << "Selected " << selection.size() << " blocks\n";
}

void BlockBuster::Editor::Editor::ClearSelection()
{
    selection.clear();
}

bool BlockBuster::Editor::Editor::CanMoveSelection(glm::ivec3 offset)
{   
    for(const auto& pair : selection)
    {
        auto pos = pair.first + offset;
        if(auto block = project.map.GetBlock(pos); block && block->type != Game::BlockType::NONE && !IsBlockInSelection(pos))
            return false;
    }

    return true;

}

bool BlockBuster::Editor::Editor::IsBlockInSelection(glm::ivec3 pos)
{
    for(const auto& pair : selection)
        if(pair.first == pos)
            return true;

    return false;
}

void BlockBuster::Editor::Editor::MoveSelection(glm::ivec3 offset)
{
    DoToolAction(std::make_unique<BlockBuster::Editor::MoveSelectionAction>(&project.map, selection, offset, &cursor.pos, &selection));
}

void BlockBuster::Editor::Editor::MoveSelectionCursor(glm::ivec3 nextPos)
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

void BlockBuster::Editor::Editor::CopySelection()
{
    clipboard = GetBlocksInSelection(false);

    std::cout << clipboard.size() << " copied to clipboard\n";
}

void BlockBuster::Editor::Editor::RemoveSelection()
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

void BlockBuster::Editor::Editor::CutSelection()
{
    CopySelection();
    RemoveSelection();
}

void BlockBuster::Editor::Editor::PasteSelection()
{
    auto batchPlace = std::make_unique<BatchedAction>();
    for(auto& bData : clipboard)
    {
        auto pos = cursor.pos + bData.first;
        if(project.map.IsBlockNull(pos))
        {
            auto placeAction = std::make_unique<PlaceBlockAction>(pos, bData.second, &project.map);
            batchPlace->AddAction(std::move(placeAction));
        }
    }
    DoToolAction(std::move(batchPlace));
}

BlockBuster::Editor::Editor::Result BlockBuster::Editor::Editor::RotateSelection(Game::RotationAxis axis, Game::RotType rotType)
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
    if(axis ==Game::RotationAxis::X)
        rotAxis = glm::vec3{1, 0, 0};
    else if(axis ==Game::RotationAxis::Z)
        rotAxis = glm::vec3{0, 0, 1};
    glm::mat3 rotMat = glm::rotate(glm::mat4{1}, glm::radians(angle), glm::vec3{rotAxis});

    // Convert to square mat
    glm::vec3 adjustedOffset = glm::vec3(cursor.scale - 1) / 2.0f;
    glm::vec3 center = glm::vec3{cursor.pos} + adjustedOffset;

    // Batched action
    auto batch = std::make_unique<BlockBuster::Editor::BatchedAction>();

    for(auto bData : lselection)
    {
        glm::vec3 offset = glm::vec3{bData.first} - center;
        glm::vec3 rotOffset = rotMat * offset;
        rotSelection.push_back({rotOffset, bData.second});

        batch->AddAction(std::make_unique<BlockBuster::Editor::RemoveAction>(bData.first, bData.second, &project.map));
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
        batch->AddAction(std::make_unique<BlockBuster::Editor::PlaceBlockAction>(absPos, bData.second, &project.map));
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

static glm::ivec3 GetRefBlock(BlockBuster::Editor::MirrorPlane plane, glm::ivec3 blockPos, glm::ivec3 cursorPos, glm::ivec3 cursorScale)
{
    glm::ivec3 refBlock;
    switch (plane)
    {
    case BlockBuster::Editor::MirrorPlane::XY:
        refBlock = glm::ivec3{blockPos.x, blockPos.y, cursorPos.z - 1};
        break;
    
    case BlockBuster::Editor::MirrorPlane::XZ:
        refBlock = glm::ivec3{blockPos.x, cursorPos.y - 1, blockPos.z};
        break;
    
    case BlockBuster::Editor::MirrorPlane::YZ:
        refBlock = glm::ivec3{cursorPos.x - 1, blockPos.y, blockPos.z};
        break;

    case BlockBuster::Editor::MirrorPlane::NOT_XY:
        refBlock = glm::ivec3{blockPos.x, blockPos.y, cursorPos.z + cursorScale.z};
        break;
    
    case BlockBuster::Editor::MirrorPlane::NOT_XZ:
        refBlock = glm::ivec3{blockPos.x, cursorPos.y + cursorScale.y, blockPos.z};
        break;
    
    case BlockBuster::Editor::MirrorPlane::NOT_YZ:
        refBlock = glm::ivec3{cursorPos.x + cursorScale.x, blockPos.y , blockPos.z};
        break;
    
    default:
        break;
    }

    return refBlock;
}

BlockBuster::Editor::Editor::Result BlockBuster::Editor::Editor::MirrorSelection(MirrorPlane plane)
{
    Result res;

    auto lselection = GetBlocksInSelection();
    auto batch = std::make_unique<BlockBuster::Editor::BatchedAction>();

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
        if(project.map.IsBlockNull(mirrorPos))
            batch->AddAction(std::make_unique<BlockBuster::Editor::PlaceBlockAction>(mirrorPos, bData.second, &project.map));
        else
            batch->AddAction(std::make_unique<BlockBuster::Editor::UpdateBlockAction>(mirrorPos, bData.second, &project.map));
    }

    DoToolAction(std::move(batch));

    return res;
}

void BlockBuster::Editor::Editor::FillSelection()
{
    auto batchPlace = std::make_unique<BatchedAction>();
    auto onEach = [this, &batchPlace](glm::ivec3 pos, glm::ivec3 offset)
    {
        if(this->project.map.IsBlockNull(pos))
        {
            Game::Block block{this->blockType, Game::BlockRot{}, this->GetBlockDisplay()};
            auto placeAction = std::make_unique<PlaceBlockAction>(pos, block, &this->project.map);
            batchPlace->AddAction(std::move(placeAction));
        }
    };
    EnumBlocksInSelection(onEach);
    DoToolAction(std::move(batchPlace));
}

void BlockBuster::Editor::Editor::ReplaceSelection()
{
    auto batchPlace = std::make_unique<BatchedAction>();
    auto onEach = [this, &batchPlace](glm::ivec3 pos, glm::ivec3 offset)
    {
        if(!this->project.map.IsBlockNull(pos))
        {
            Game::Block block{this->blockType, Game::BlockRot{}, this->GetBlockDisplay()};
            auto placeAction = std::make_unique<UpdateBlockAction>(pos, block, &this->project.map);
            batchPlace->AddAction(std::move(placeAction));
        }
    };
    EnumBlocksInSelection(onEach);
    DoToolAction(std::move(batchPlace));
}

void BlockBuster::Editor::Editor::PaintSelection()
{
    auto batchPlace = std::make_unique<BatchedAction>();
    auto onEach = [this, &batchPlace](glm::ivec3 pos, glm::ivec3 offset)
    {
        if(!this->project.map.IsBlockNull(pos))
        {
            auto block = this->project.map.GetBlock(pos);
            auto display = this->GetBlockDisplay();
            auto placeAction = std::make_unique<PaintAction>(pos, block, display, &this->project.map);
            batchPlace->AddAction(std::move(placeAction));
        }
    };
    EnumBlocksInSelection(onEach);
    DoToolAction(std::move(batchPlace));
}


void BlockBuster::Editor::Editor::OnChooseSelectSubTool(SelectSubTool subTool)
{
    // Always stop moving selection
    movingSelection = false;
    ClearSelection();
}

void BlockBuster::Editor::Editor::HandleKeyShortCut(const SDL_KeyboardEvent& key)
{
    auto& io = ImGui::GetIO();
    if(key.type == SDL_KEYDOWN)
    {
        auto sym = key.keysym.sym;
        if(sym == SDLK_ESCAPE)
        {
            if(state != PopUpState::NONE)
                ClosePopUp();
            else
                Exit();
        }

        if(state != PopUpState::NONE)
            return;

        // Select tool
        if(tool == Tool::SELECT_BLOCKS)
        {
            auto nextPos = cursor.pos;
            auto scale = cursor.scale;

            auto moveOrScale = [&nextPos, &scale, &sym, this](Sint32 key, glm::ivec3 offset)
            {
                if(sym == key)
                {
                    if(!this->io_->KeyCtrl)
                        nextPos += offset;
                    else if(!movingSelection)
                        scale += offset;
                }
            };

            // X axis
            moveOrScale(SDLK_KP_4, glm::ivec3{-1, 0, 0});
            moveOrScale(SDLK_KP_6, glm::ivec3{1, 0, 0});

            // Y axis
            moveOrScale(SDLK_KP_7, glm::ivec3{0, 1, 0});
            moveOrScale(SDLK_KP_9, glm::ivec3{0, -1, 0});

            // Z axis
            moveOrScale(SDLK_KP_8, glm::ivec3{0, 0, -1});
            moveOrScale(SDLK_KP_2, glm::ivec3{0, 0, 1});

            bool hasMoved = nextPos != cursor.pos;
            cursor.scale = glm::max(scale, glm::ivec3{1, 1, 1});
            if(hasMoved)
                MoveSelectionCursor(nextPos);
        }

        // Camera mode
        if(sym == SDLK_f)
        {
            auto mode = cameraMode == CameraMode::EDITOR ? CameraMode::FPS : CameraMode::EDITOR;
            SetCameraMode(mode);
        }
        
        // Editor/Player Mode toggle
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

        // File Menu
        if(sym == SDLK_n && io.KeyCtrl)
            MenuNewMap();

        if(sym == SDLK_s && io.KeyCtrl)
            MenuSave();

        if(sym == SDLK_s && io.KeyCtrl && io.KeyShift)
            MenuSaveAs();

        if(sym == SDLK_o && io.KeyCtrl)
            MenuOpenMap();

        // Edit
        if(sym == SDLK_z && io.KeyCtrl && !io.KeyShift)
            UndoToolAction();

        if(sym == SDLK_z && io.KeyCtrl && io.KeyShift)
            DoToolAction();

        if(sym == SDLK_g && io.KeyCtrl)
        {
            OpenPopUp(PopUpState::GO_TO_BLOCK);
        }

        // Editor navigation
        if(sym >= SDLK_1 &&  sym <= SDLK_4 && io.KeyCtrl)
            SelectTool(static_cast<Tool>(sym - SDLK_1));

        // Select display/blocktype/axis
        if(sym >= SDLK_1 && sym <= SDLK_2 && !io.KeyCtrl && !io.KeyAlt)
        {
            if(tool == Tool::PLACE_BLOCK)
                blockType = static_cast<Game::BlockType>(sym - SDLK_0);
            if(tool == Tool::ROTATE_BLOCK)
                axis = static_cast<Game::RotationAxis>(sym - SDLK_0);
            if(tool == Tool::PAINT_BLOCK)
                displayType = static_cast<Game::DisplayType>(sym - SDLK_1);
        }

        if(sym >= SDLK_1 && sym <= SDLK_3 && !io.KeyCtrl && io.KeyAlt)
            tabState = static_cast<TabState>(sym - SDLK_1);
    }
}

Game::Display BlockBuster::Editor::Editor::GetBlockDisplay()
{
    Game::Display display;
    display.type = displayType;
    if(displayType == Game::DisplayType::COLOR)
        display.id = colorId;
    else if(displayType == Game::DisplayType::TEXTURE)
        display.id = textureId;

    return display;
}

bool BlockBuster::Editor::Editor::IsDisplayValid()
{
    auto display = GetBlockDisplay();
    auto texCount = project.tPalette.GetCount();
    bool isValidTexture = display.type == Game::DisplayType::TEXTURE && display.id < texCount;
    bool isColor = display.type == Game::DisplayType::COLOR;
    return isColor || isValidTexture;
}

void BlockBuster::Editor::Editor::SetBlockDisplay(Game::Display display)
{
    displayType = display.type;
    if(displayType == Game::DisplayType::TEXTURE)
        textureId = display.id;
    else if(displayType == Game::DisplayType::COLOR)
        colorId = display.id;
}

void BlockBuster::Editor::Editor::SetUnsaved(bool unsaved)
{
    this->unsaved = unsaved;
    if(unsaved)
        RenameMainWindow(fileName + std::string("*"));
    else
        RenameMainWindow(fileName);
}

void BlockBuster::Editor::Editor::Exit()
{
    if(unsaved)
    {
        if(playerMode)
            playerMode = false;

        OpenWarningPopUp(std::bind(&BlockBuster::Editor::Editor::Exit, this));
        if(cameraMode != CameraMode::EDITOR)
            SetCameraMode(CameraMode::EDITOR);
    }
    else
        quit = true;
}

// #### Test Mode #### \\

void BlockBuster::Editor::Editor::UpdatePlayerMode()
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
    player.HandleCollisions(&project.map, blockScale);
    auto playerPos = player.transform.position;

    auto cameraPos = playerPos + glm::vec3{0.0f, player.height, 0.0f};
    camera.SetPos(cameraPos);
}

// #### Options #### \\

void BlockBuster::Editor::Editor::HandleWindowEvent(SDL_WindowEvent winEvent)
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

void BlockBuster::Editor::Editor::ApplyVideoOptions(::App::Configuration::WindowConfig& winConfig)
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
    camera.SetParam(Rendering::Camera::Param::FOV, winConfig.fov);
    SDL_GL_SetSwapInterval(winConfig.vsync);

    SDL_SetWindowPosition(window_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

std::string BlockBuster::Editor::Editor::GetConfigOption(const std::string& key, std::string defaultValue)
{
    std::string ret = defaultValue;
    auto it = config.options.find(key);

    if(it != config.options.end())
    {
        ret = it->second;
    }

    return ret;
}

