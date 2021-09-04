#include <Editor.h>

#include <game/Game.h>
#include <math/BBMath.h>

#include <iostream>
#include <algorithm>
#include <fstream>
#include <cstring>

#include <glm/gtc/constants.hpp>

#include <debug/Debug.h>

// #### Public Interface #### \\

void BlockBuster::Editor::Editor::Start()
{
    // Shaders
    shader.Use();

    // Meshes
    cube = Rendering::Primitive::GenerateCube();
    slope = Rendering::Primitive::GenerateSlope();

    // Textures
    textures.reserve(MAX_TEXTURES);

    textureFolder = GetConfigOption("TextureFolder", TEXTURES_DIR);
    GL::Texture texture = GL::Texture::FromFolder(textureFolder, "SmoothStone.png");
    try
    {
        texture.Load();
        textures.push_back(std::move(texture));
    }
    catch(const GL::Texture::LoadError& e)
    {
        std::cout << "Error when loading texture " + e.path_.string() + ": " +  e.what() << '\n';
    }
    
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
        mapLoaded = OpenProject();
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
    // Clear Buffer
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw new Map System Cubes
    auto iterator = map_.CreateIterator();
    for(auto b = iterator.GetNextBlock(); !iterator.IsOver(); b = iterator.GetNextBlock())
    {
        auto block = b.second;
        auto pos = b.first;
        Math::Transform t = Game::GetBlockTransform(*block, pos, blockScale);
        auto mMat = t.GetTransformMat();
        auto tMat = camera.GetProjViewMat() * mMat;

        shader.SetUniformInt("isPlayer", 0);
        shader.SetUniformInt("hasBorder", true);
        shader.SetUniformMat4("transform", tMat);
        auto& mesh = GetMesh(block->type);
        auto display = tool == PAINT_BLOCK && intersecting && pointedBlockPos == pos ? GetBlockDisplay() : block->display;
        if(display.type == Game::DisplayType::TEXTURE)
        {
            if(display.id < textures.size())
                mesh.Draw(shader, &textures[display.id]);
        }
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

bool BlockBuster::Editor::Editor::LoadTexture()
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

bool BlockBuster::Editor::Editor::IsTextureInPalette(std::filesystem::path folder, std::filesystem::path textureName)
{
    auto texturePath = folder / textureName;
    for(const auto& texture : textures)
        if(texture.GetPath() == texturePath)
            return true;
    
    return false;
}

// #### World #### \\

void BlockBuster::Editor::Editor::NewProject()
{
    // Init project
    project = Project{};
    project.Init();

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
}

void BlockBuster::Editor::Editor::SaveProject()
{
    std::filesystem::path mapPath = mapsFolder / fileName;

    // Write project file
    project.cameraPos = camera.GetPos();
    project.cameraRot = camera.GetRotation();
    project.textureFolder = textureFolder;
    ::BlockBuster::Editor::WriteProjectToFile(project, mapPath);

    RenameMainWindow(fileName);

    // Update flag
    newMap = false;
    unsaved = false;
}

bool BlockBuster::Editor::Editor::OpenProject()
{
    std::filesystem::path mapPath = mapsFolder / fileName;
    Project temp = ::BlockBuster::Editor::ReadProjectFromFile(mapPath);
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
        colorPick = colors[colorId];

        // Clear history
        ClearActionHistory();

        // Set camera pos
        camera.SetPos(project.cameraPos);
        camera.SetRotation(project.cameraRot.x, project.cameraRot.y);
    }

    return isOk;
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
        auto chunkIt = map_.CreateChunkIterator();
        for(auto chunk = chunkIt.GetNextChunk(); !chunkIt.IsOver(); chunk = chunkIt.GetNextChunk())
        {
            auto pos = Game::Map::ToRealChunkPos(chunk.first, blockScale);
            auto size = glm::vec3{Game::Map::Map::Chunk::DIMENSIONS} * blockScale;
            Math::Transform ct{pos, glm::vec3{0.0f}, size};
            auto model = ct.GetTransformMat();

            auto transform = camera.GetProjViewMat() * model;
            shader.SetUniformMat4("transform", transform);
            shader.SetUniformInt("hasBorder", false);
            auto& mesh = GetMesh(cursor.type);
            mesh.Draw(shader, yellow, GL_LINE);
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
        intersect = Game::CastRayFirst(&map_, ray, blockScale);
#ifdef _DEBUG
    }
    else
    {
        auto intersections = Game::CastRay(&map_, ray, blockScale);
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
                        auto display = GetBlockDisplay();                            
                        auto iNewPos = intersect.pos + glm::ivec3{glm::round(intersection.normal)};
                        if(auto found = map_.GetBlock(iNewPos); !found || found->type == Game::BlockType::NONE)
                        {
                            auto block = Game::Block{blockType, rot, display};
                            auto action = std::make_unique<PlaceBlockAction>(iNewPos, block, &map_);

                            DoToolAction(std::move(action));
                        }
                    }
                    else if(actionType == ActionType::HOVER)
                    {
                        cursor.pos = intersect.pos + glm::ivec3{glm::round(intersection.normal)};
                        if(auto found = map_.GetBlock(cursor.pos); !found || found->type == Game::BlockType::NONE)
                        {
                            cursor.enabled = true;
                            cursor.type = blockType;
                            cursor.rot = rot;
                            if(block.display.type == Game::DisplayType::COLOR)
                            {
                                cursor.color = GetBorderColor(colors[block.display.id], darkBlue, yellow);
                            }
                            else
                                cursor.color = yellow;
                        }
                        else
                            cursor.enabled = false;
                    }
                }
                else if(actionType == ActionType::RIGHT_BUTTON)
                {
                    if(map_.GetBlockCount() > 1)
                    {
                        auto action = std::make_unique<RemoveAction>(intersect.pos, block, &map_);
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

                        DoToolAction(std::make_unique<RotateAction>(intersect.pos, &block, &map_, blockRot));
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
            if(intersection.intersects)
            {
                auto block = *intersect.block;
                auto iPos = intersect.pos;
                if(actionType == ActionType::LEFT_BUTTON)
                {
                    auto display = GetBlockDisplay();
                    DoToolAction(std::make_unique<PaintAction>(iPos, &block, display, &map_));
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
    auto& mesh = GetMesh(cursor.type);
    mesh.Draw(shader, cursor.color, GL_LINE);
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

std::vector<BlockBuster::Editor::BlockData> BlockBuster::Editor::Editor::GetBlocksInSelection(bool globalPos)
{
    std::vector<BlockBuster::Editor::BlockData> selection;

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
                if(!map_.IsBlockNull(ipos))
                {
                    if(globalPos)
                        selection.push_back({ipos, *map_.GetBlock(ipos)});
                    else
                        selection.push_back({offset, *map_.GetBlock(ipos)});
                }
            }
        }
    }

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
        if(auto block = map_.GetBlock(pos); block && block->type != Game::BlockType::NONE && !IsBlockInSelection(pos))
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
    DoToolAction(std::make_unique<BlockBuster::Editor::MoveSelectionAction>(&map_, selection, offset, &cursor.pos));

    std::cout << "Moving selection\n";
}

void BlockBuster::Editor::Editor::MoveSelectionCursor(glm::ivec3 nextPos)
{
    if(movingSelection)
    {
        SelectBlocks();

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
        auto removeAction = std::make_unique<RemoveAction>(bData.first, bData.second, &map_);
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
        if(map_.IsBlockNull(pos))
        {
            std::cout << "Block is null\n";
            auto placeAction = std::make_unique<PlaceBlockAction>(pos, bData.second, &map_);
            batchPlace->AddAction(std::move(placeAction));
        }
    }
    DoToolAction(std::move(batchPlace));
}

// TODO: Fix rotation in X
// TODO: Fix 180 rotations in Z
BlockBuster::Editor::Editor::Result BlockBuster::Editor::Editor::RotateSelection(BlockBuster::Editor::Editor::RotationAxis axis, Game::RotType rotType)
{
    Result res;

    //Check cursor is a square in the involved axis;
    if(rotType == Game::RotType::ROT_90)
    {
        bool canRotateX = axis == RotationAxis::X && cursor.scale.y == cursor.scale.z;
        bool canRotateY = axis == RotationAxis::Y && cursor.scale.x == cursor.scale.z;
        bool canRotateZ = axis == RotationAxis::Z && cursor.scale.x == cursor.scale.y;
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
    if(axis == RotationAxis::X)
        rotAxis = glm::vec3{1, 0, 0};
    else if(axis == RotationAxis::Z)
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

        batch->AddAction(std::make_unique<BlockBuster::Editor::RemoveAction>(bData.first, bData.second, &map_));
    }

    for(auto bData : rotSelection)
    {
        glm::ivec3 absPos = glm::round(center + bData.first);
        if(bData.second.type == Game::BlockType::SLOPE)
        {
            if(axis == RotationAxis::Y)
            {
                bData.second.rot = GetNextValidRotation(bData.second.rot, axis, true);
                if(rotType == Game::RotType::ROT_180)
                    bData.second.rot = GetNextValidRotation(bData.second.rot, axis, true);
            }
            else if(axis == RotationAxis::Z)
            {
                auto bRot = bData.second.rot;
                if(bRot.y == Game::RotType::ROT_0)
                    bData.second.rot = GetNextValidRotation(bData.second.rot, axis, true);
                else if(bRot.y == Game::RotType::ROT_90)
                {
                    if(bRot.z == Game::RotType::ROT_180)
                    {
                        bData.second.rot = GetNextValidRotation(bData.second.rot, axis, true);
                        bData.second.rot = GetNextValidRotation(bData.second.rot, axis, true);
                    }
                    else
                    {
                        bData.second.rot = GetNextValidRotation(bData.second.rot, RotationAxis::Y, true);
                        bData.second.rot = GetNextValidRotation(bData.second.rot, RotationAxis::Y, true);
                    }
                }
                else if(bRot.y == Game::RotType::ROT_180)
                    bData.second.rot = GetNextValidRotation(bData.second.rot, axis, false);
                else if(bRot.y == Game::RotType::ROT_270)
                {
                    if(bRot.z == Game::RotType::ROT_180)
                    {
                        bData.second.rot = GetNextValidRotation(bData.second.rot, RotationAxis::Y, true);
                        bData.second.rot = GetNextValidRotation(bData.second.rot, RotationAxis::Y, true);
                    }
                    else
                    {
                        bData.second.rot = GetNextValidRotation(bData.second.rot, axis, true);
                        bData.second.rot = GetNextValidRotation(bData.second.rot, axis, true);
                    }
                    
                }
            }
        }

        batch->AddAction(std::make_unique<BlockBuster::Editor::PlaceBlockAction>(absPos, bData.second, &map_));
    }

    if(rotType == Game::ROT_90)
    {
        // Rotate scale
        auto cs = cursor.scale;
        cursor.scale = axis == RotationAxis::Y ? glm::ivec3{cs.z, cs.y, cs.x} : glm::ivec3{cs.y, cs.x, cs.z};
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

// TODO: Fix slope rotations
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
            if(plane != MirrorPlane::YZ && plane != MirrorPlane::NOT_YZ)
            {
                auto axis = plane == MirrorPlane::XY  || plane == MirrorPlane::NOT_XY ? RotationAxis::Y : RotationAxis::Z;
                auto rot = GetNextValidRotation(bData.second.rot, axis, true);
                rot = GetNextValidRotation(rot, axis, true);
                bData.second.rot = rot;
            }

        }
        batch->AddAction(std::make_unique<BlockBuster::Editor::PlaceBlockAction>(mirrorPos, bData.second, &map_));
    }

    DoToolAction(std::move(batch));

    return res;
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
            // TODO: Error when closing pop up without calling OnClose code. Could use a PopUp class for this
            if(state != PopUpState::NONE)
                ClosePopUp();
            else
                Exit();
        }

        if(state != PopUpState::NONE)
            return;

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

        if(sym >= SDLK_1 && sym <= SDLK_2 && !io.KeyCtrl && !io.KeyAlt)
        {
            if(tool == Tool::PLACE_BLOCK)
                blockType = static_cast<Game::BlockType>(sym - SDLK_0);
            if(tool == Tool::ROTATE_BLOCK)
                axis = static_cast<RotationAxis>(sym - SDLK_0);
            if(tool == Tool::PAINT_BLOCK)
                displayType = static_cast<Game::DisplayType>(sym - SDLK_1);
        }

        if(sym >= SDLK_1 && sym <= SDLK_2 && !io.KeyCtrl && io.KeyAlt)
            tabState = static_cast<TabState>(sym - SDLK_1)        ;
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

Game::BlockRot BlockBuster::Editor::Editor::GetNextValidRotation(Game::BlockRot rot, RotationAxis axis, bool positive)
{
    Game::BlockRot blockRot = rot;
    auto sign = positive ? 1 : -1;
    if(axis == RotationAxis::Y)
    {
        int8_t i8rot = Math::OverflowSumInt<int8_t>(blockRot.y, sign, Game::RotType::ROT_0, Game::RotType::ROT_270);
        blockRot.y = static_cast<Game::RotType>(i8rot);
    }
    else if(axis == RotationAxis::Z)
    {
        int8_t i8rot = Math::OverflowSumInt<int8_t>(blockRot.z, sign, Game::RotType::ROT_0, Game::RotType::ROT_270);
        blockRot.z = static_cast<Game::RotType>(i8rot);
    }

    return blockRot;
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
    player.HandleCollisions(&map_, blockScale);
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

// #### GUI - PopUps #### \\

void BlockBuster::Editor::Editor::EditTextPopUp(const EditTextPopUpParams& params)
{
    auto displaySize = io_->DisplaySize;
    ImGui::SetNextWindowPos(ImVec2{displaySize.x * 0.5f, displaySize.y * 0.5f}, ImGuiCond_Always, ImVec2{0.5f, 0.5f});
    bool onPopUp = state == params.popUpState;
    if(ImGui::BeginPopupModal(params.name.c_str(), &onPopUp, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
    {   
        bool accept = ImGui::InputText("File name", params.textBuffer, params.bufferSize, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
        bool ok = true;
        if(ImGui::Button("Accept") || accept)
        {
            ok = params.onAccept();

            if(ok)
            {
                params.onError = false;
                
                ClosePopUp();
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

            ClosePopUp();
        }

        if(params.onError)
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", params.errorText.c_str());

        ImGui::EndPopup();
    }
}

void BlockBuster::Editor::Editor::BasicPopUp(const BasicPopUpParams& params)
{
    bool onPopUp = this->state == params.state;
    if(ImGui::BeginPopupModal(params.name.c_str(), &onPopUp, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
    {
        params.inPopUp();

        ImGui::EndPopup();
    }
    else if(this->state == params.state)
    {
        ClosePopUp();
    }
}

void BlockBuster::Editor::Editor::OpenMapPopUp()
{
    std::string errorPrefix = "Could not open map ";
    auto onAccept = std::bind(&BlockBuster::Editor::Editor::OpenProject, this);
    auto onCancel = [](){};
    EditTextPopUpParams params{PopUpState::OPEN_MAP, "Open Map", fileName, 32, onAccept, onCancel, errorPrefix, onError, errorText};
    EditTextPopUp(params);
}

void BlockBuster::Editor::Editor::SaveAsPopUp()
{
    std::string errorPrefix = "Could not save map ";
    auto onAccept = [this](){this->SaveProject(); return true;};
    auto onCancel = [](){};
    EditTextPopUpParams params{PopUpState::SAVE_AS, "Save As", fileName, 32, onAccept, onCancel, errorPrefix, onError, errorText};
    EditTextPopUp(params);
}

void BlockBuster::Editor::Editor::LoadTexturePopUp()
{
    std::string errorPrefix = "Could not open texture ";
    auto onAccept = std::bind(&BlockBuster::Editor::Editor::LoadTexture, this);
    auto onCancel = [](){};
    EditTextPopUpParams params{PopUpState::LOAD_TEXTURE, "Load Texture", textureFilename, 32, onAccept, onCancel, errorPrefix, onError, errorText};
    EditTextPopUp(params);
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

void BlockBuster::Editor::Editor::VideoOptionsPopUp()
{
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

        int fov = glm::degrees(preConfig.fov);
        if(ImGui::SliderInt("FOV", &fov, 45, 90))
        {
            preConfig.fov = glm::radians((float)fov);
            std::cout << "Preconfig fov is " << preConfig.fov << "\n";
        }

        if(ImGui::Button("Accept"))
        {
            ApplyVideoOptions(preConfig);
            config.window = preConfig;

            ClosePopUp(true);
        }

        ImGui::SameLine();
        if(ImGui::Button("Apply"))
        {
            ApplyVideoOptions(preConfig);
        }

        ImGui::SameLine();
        if(ImGui::Button("Cancel"))
        {
            ClosePopUp(false);
        }

        ImGui::EndPopup();
    }
    // Triggered when X button is pressed, same effect as cancel
    else if(state == PopUpState::VIDEO_SETTINGS)
    {
        ClosePopUp(false);
    }
}

void BlockBuster::Editor::Editor::UnsavedWarningPopUp()
{
    auto displaySize = io_->DisplaySize;
    ImGui::SetNextWindowPos(ImVec2{displaySize.x * 0.5f, displaySize.y * 0.5f}, ImGuiCond_Always, ImVec2{0.5f, 0.5f});

    bool onPopUp = state == PopUpState::UNSAVED_WARNING;
    if(ImGui::BeginPopupModal(popUps[UNSAVED_WARNING].name.c_str(), &onPopUp, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("There's unsaved content.\nWhat would you like to do?");

        if(ImGui::Button("Save"))
        {
            SaveProject();
            ClosePopUp();
            onWarningExit();
        }
        ImGui::SameLine();

        if(ImGui::Button("Don't Save"))
        {
            unsaved = false;
            ClosePopUp();

            onWarningExit();
        }
        ImGui::SameLine();

        if(ImGui::Button("Cancel"))
        {
            ClosePopUp();
        }

        ImGui::EndPopup();
    }
    else if(state == PopUpState::UNSAVED_WARNING)
    {
        ClosePopUp();
    }
}

void BlockBuster::Editor::Editor::GoToBlockPopUp()
{
    auto inPopUp = [this](){
        
        ImGui::InputInt3("Position", &goToPos.x);

        if(ImGui::Button("Accept"))
        {
            glm::vec3 cameraPos = (glm::vec3)this->goToPos * this->blockScale;
            this->camera.SetPos(cameraPos);
            ClosePopUp();
        }

        ImGui::SameLine();
        if(ImGui::Button("Cancel"))
        {
            ClosePopUp();
        }
    };
    BasicPopUp({PopUpState::GO_TO_BLOCK, "Go to block", inPopUp});
}

void BlockBuster::Editor::Editor::OpenWarningPopUp(std::function<void()> onExit)
{
    OpenPopUp(PopUpState::UNSAVED_WARNING);
    onWarningExit = onExit;
}

void BlockBuster::Editor::Editor::InitPopUps()
{
    popUps[NONE].update = []{};

    popUps[SAVE_AS].name = "Save As";
    popUps[SAVE_AS].update = std::bind(&BlockBuster::Editor::Editor::SaveAsPopUp, this);

    popUps[OPEN_MAP].name = "Open Map";
    popUps[OPEN_MAP].update = std::bind(&BlockBuster::Editor::Editor::OpenMapPopUp, this);

    popUps[LOAD_TEXTURE].name = "Load Texture";
    popUps[LOAD_TEXTURE].update = std::bind(&BlockBuster::Editor::Editor::LoadTexturePopUp, this);

    popUps[UNSAVED_WARNING].name = "Unsaved content";
    popUps[UNSAVED_WARNING].update = std::bind(&BlockBuster::Editor::Editor::UnsavedWarningPopUp, this);

    popUps[VIDEO_SETTINGS].name = "Video";
    popUps[VIDEO_SETTINGS].update = std::bind(&BlockBuster::Editor::Editor::VideoOptionsPopUp, this);
    popUps[VIDEO_SETTINGS].onOpen = [this](){
        std::cout << "Loading config\n";
        preConfig = config.window;
    };
    popUps[VIDEO_SETTINGS].onClose = [this](bool accept){
        if(!accept)
        {
            ApplyVideoOptions(config.window);
            preConfig = config.window;
        }
    };

    popUps[GO_TO_BLOCK].name = "Go to block";
    popUps[GO_TO_BLOCK].onOpen = [this](){
        this->goToPos = this->camera.GetPos() / this->blockScale;
    };
    popUps[GO_TO_BLOCK].update = std::bind(&BlockBuster::Editor::Editor::GoToBlockPopUp, this);
}

void BlockBuster::Editor::Editor::OpenPopUp(PopUpState puState)
{
    if(state == PopUpState::NONE)
        this->state = puState;
}

void BlockBuster::Editor::Editor::UpdatePopUp()
{
    // Open if not open yet
    auto name = popUps[state].name.c_str();
    bool isOpen = ImGui::IsPopupOpen(name);
    if(this->state != PopUpState::NONE && !isOpen)
    {
        popUps[state].onOpen();
        ImGui::OpenPopup(name);
    }

    // Render each popup data
    for(int s = NONE; s < MAX; s++)
    {
        popUps[s].update();
    }
}

void BlockBuster::Editor::Editor::ClosePopUp(bool accept)
{
    popUps[state].onClose(accept);
    this->state = NONE;
    ImGui::CloseCurrentPopup();
}

// #### GUI #### \\

void BlockBuster::Editor::Editor::MenuBar()
{
    // Pop Ups
    UpdatePopUp();

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
                UndoToolAction();
            }

            if(ImGui::MenuItem("Redo", "Ctrl + Shift + Z"))
            {
                DoToolAction();
                std::cout << "Redoing\n";
            }

            ImGui::Separator();

            if(ImGui::MenuItem("Go to Block", "Ctrl + G"))
            {
                OpenPopUp(PopUpState::GO_TO_BLOCK);
                std::cout << "Going to block\n";
            }

            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("Settings", true))
        {
            if(ImGui::MenuItem("Video", "Ctrl + Shift + G"))
            {
                OpenPopUp(PopUpState::VIDEO_SETTINGS);
            }

            #ifdef _DEBUG
            ImGui::Checkbox("Show demo window", &showDemo);
            #endif

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

// #### GUI - File Menu #### \\

void BlockBuster::Editor::Editor::MenuNewMap()
{
    if(unsaved)
    {
        auto onExit = std::bind(&BlockBuster::Editor::Editor::NewProject, this);
        OpenWarningPopUp(onExit);
    }
    else
        NewProject();
}

void BlockBuster::Editor::Editor::MenuOpenMap()
{
    if(unsaved)
    {
        auto onExit = [this](){OpenPopUp(PopUpState::OPEN_MAP);};
        OpenWarningPopUp(onExit);
    }
    else
        OpenPopUp(PopUpState::OPEN_MAP);
}

void BlockBuster::Editor::Editor::MenuSave()
{
    if(newMap)
        OpenPopUp(PopUpState::SAVE_AS);
    else
        SaveProject();
}

void BlockBuster::Editor::Editor::MenuSaveAs()
{
    OpenPopUp(PopUpState::SAVE_AS);
}

// #### GUI - Base #### \\

void BlockBuster::Editor::Editor::GUI()
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
                bool selectSelected = tool == SELECT_BLOCKS;

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
                            SelectTool(PLACE_BLOCK);
                        }

                        ImGui::TableNextColumn();
                        if(ImGui::Selectable("Rotate", &rotbSelected, 0, ImVec2{0, 0}))
                        {
                            SelectTool(ROTATE_BLOCK);
                        }

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        if(ImGui::Selectable("Paint", &paintSelected, 0, ImVec2{0, 0}))
                        {
                            SelectTool(PAINT_BLOCK);
                        }

                        ImGui::TableNextColumn();
                        if(ImGui::Selectable("Select", selectSelected, 0, ImVec2{0, 0}))
                        {
                            SelectTool(SELECT_BLOCKS);
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
                                OpenPopUp(PopUpState::LOAD_TEXTURE);
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
                    }

                    if(selectSelected)
                    {
                        ImGui::Text("Cursor Display");
                        ImGui::SameLine();
                        ImGui::RadioButton("Single Block", &cursor.mode, CursorMode::SCALED);
                        ImGui::SameLine();
                        ImGui::RadioButton("Multiple Blocks", &cursor.mode, CursorMode::BLOCKS);

                        if(ImGui::BeginTable("##Select Scale", 4))
                        {
                            ImGui::TableSetupColumn("##Title", ImGuiTableColumnFlags_WidthFixed, 60);
                            ImGui::TableNextColumn();

                            ImGui::Text("Scale");
                            ImGui::TableNextColumn();
                            
                            auto flag = movingSelection ? ImGuiInputTextFlags_ReadOnly : 0;
                            ImGui::InputInt("X", &cursor.scale.x, 1, 1, flag);
                            ImGui::TableNextColumn();
                            ImGui::InputInt("Y", &cursor.scale.y, 1, 1, flag);
                            ImGui::TableNextColumn();
                            ImGui::InputInt("Z", &cursor.scale.z, 1, 1, flag);
                            ImGui::TableNextColumn();

                            cursor.scale = glm::max(cursor.scale, glm::ivec3{1, 1, 1});

                            ImGui::EndTable();
                        }

                        if(ImGui::BeginTable("##Select Pos", 4))
                        {
                            ImGui::TableSetupColumn("##Title", ImGuiTableColumnFlags_WidthFixed, 60);
                            ImGui::TableNextColumn();

                            ImGui::Text("Position");
                            ImGui::TableNextColumn();

                            auto nextPos = cursor.pos;
                            bool hasSelectionMoved = false;
                            hasSelectionMoved |= ImGui::InputInt("X", &nextPos.x, 1);
                            ImGui::TableNextColumn();
                            hasSelectionMoved |= ImGui::InputInt("Y", &nextPos.y, 1);
                            ImGui::TableNextColumn();
                            hasSelectionMoved |= ImGui::InputInt("Z", &nextPos.z, 1);
                            ImGui::TableNextColumn();
                            if(hasSelectionMoved)
                                MoveSelectionCursor(nextPos);

                            ImGui::EndTable();
                        }
                        
                        if(ImGui::BeginTable("###Select tools", 2, 0))
                        {
                            ImGui::TableSetupColumn("##Sub-Tools", ImGuiTableColumnFlags_WidthFixed);
                            ImGui::TableSetupColumn("##Tool Options", 0);

                            ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TableHeader("##Sub-Tools");
                            ImGui::SameLine();
                            ImGui::Text("Sub-Tools");

                            ImGui::TableSetColumnIndex(1);
                            ImGui::TableHeader("##Tool Options");
                            ImGui::SameLine();
                            ImGui::Text("Tools Options");

                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();

                            auto cursorX = ImGui::GetCursorPosX();
                            ImGui::SetCursorPosX(cursorX + 4);
                            if(ImGui::BeginTable("###Select Tools Pad", 2, 0, ImVec2{100, 0}))
                            {
                                ImGui::TableNextColumn();
                                ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_SelectableTextAlign, {0.5, 0});
                                if(ImGui::Selectable("Move", &selectTool, SelectSubTool::MOVE))
                                    OnChooseSelectSubTool(selectTool);

                                ImGui::TableNextColumn();
                                if(ImGui::Selectable("Edit", &selectTool, SelectSubTool::EDIT))
                                    OnChooseSelectSubTool(selectTool);

                                ImGui::TableNextColumn();
                                if(ImGui::Selectable("Rotate", &selectTool, SelectSubTool::ROTATE_OR_MIRROR))
                                    OnChooseSelectSubTool(selectTool);

                                ImGui::PopStyleVar();

                                

                                ImGui::EndTable();
                            }

                            ImGui::TableNextColumn();
                            switch (selectTool)
                            {
                                case SelectSubTool::MOVE:
                                {
                                    const char* label = movingSelection ? "Stop Moving" : "Start Moving";
                                    ImVec4 color = movingSelection ? ImVec4{1.0f, 0.0f, 0.0f, 1.0f} : ImVec4{0.0f, 1.0f, 0.0f, 1.0f};
                                    ImGui::PushStyleColor(ImGuiCol_Button, color);
                                    if(ImGui::Button(label))
                                    {
                                        movingSelection = !movingSelection;
                                        if(movingSelection)
                                            SelectBlocks();
                                        else
                                            ClearSelection();
                                    }
                                    ImGui::PopStyleColor();

                                    #ifdef _DEBUG
                                    ImGui::Text("Selected %zu blocks", selection.size());
                                    ImVec2 size {0, 40};
                                    if(ImGui::BeginTable("Selection", 1, ImGuiTableFlags_Hideable | ImGuiTableFlags_ScrollY, size))
                                    {
                                        for(auto pair : selection)
                                        {
                                            ImGui::TableNextColumn();
                                            std::string str = "Block pos: " + Debug::ToString(pair.first);
                                            ImGui::Text("%s", str.c_str());
                                        }
                                        ImGui::EndTable();
                                    }
                                    #endif
                                }
                                break;
                                
                                case SelectSubTool::EDIT:
                                {
                                    if(ImGui::Button("Copy"))
                                    {
                                        CopySelection();
                                    }
                                    ImGui::SameLine();

                                    if(ImGui::Button("Cut"))
                                    {
                                        CutSelection();
                                    }
                                    ImGui::SameLine();

                                    if(ImGui::Button("Paste"))
                                    {
                                        PasteSelection();
                                    }
                                    ImGui::SameLine();

                                    if(ImGui::Button("Remove"))
                                    {
                                        RemoveSelection();
                                    }
                                    ImGui::SameLine();
                                    break;
                                }

                                case SelectSubTool::ROTATE_OR_MIRROR:
                                {
                                    ImGui::Text("Rotate");
                                    ImGui::Separator();

                                    ImGui::Text("Axis");

                                    ImGui::SameLine();
                                    ImGui::RadioButton("X", &selectRotAxis, RotationAxis::X);
                                    ImGui::SameLine();
                                    ImGui::RadioButton("Y", &selectRotAxis, RotationAxis::Y);
                                    ImGui::SameLine();
                                    ImGui::RadioButton("Z", &selectRotAxis, RotationAxis::Z);

                                    ImGui::Text("Rotation angle (deg)");

                                    ImGui::SameLine();
                                    ImGui::RadioButton("90", &selectRotType, Game::RotType::ROT_90);
                                    ImGui::SameLine();
                                    ImGui::RadioButton("180", &selectRotType, Game::RotType::ROT_180);

                                    if(ImGui::Button("Rotate"))
                                    {
                                        auto res = RotateSelection(selectRotAxis, selectRotType);
                                        selectRotErrorText = res.info;
                                    }

                                    ImGui::Text("Mirror");
                                    ImGui::Separator();

                                    ImGui::Text("Plane");

                                    ImGui::SameLine();
                                    ImGui::RadioButton("XY", &selectMirrorPlane, MirrorPlane::XY);
                                    ImGui::SameLine();
                                    ImGui::RadioButton("XZ", &selectMirrorPlane, MirrorPlane::XZ);
                                    ImGui::SameLine();
                                    ImGui::RadioButton("YZ", &selectMirrorPlane, MirrorPlane::YZ);
                                    ImGui::SameLine();
                                    ImGui::RadioButton("-XY", &selectMirrorPlane, MirrorPlane::NOT_XY);
                                    ImGui::SameLine();
                                    ImGui::RadioButton("-XZ", &selectMirrorPlane, MirrorPlane::NOT_XZ);
                                    ImGui::SameLine();
                                    ImGui::RadioButton("-YZ", &selectMirrorPlane, MirrorPlane::NOT_YZ);


                                    if(ImGui::Button("Mirror"))
                                    {
                                        auto res = MirrorSelection(selectMirrorPlane);
                                        selectRotErrorText = res.info;
                                    }

                                    // Error text
                                    ImVec4 red{1.0f, 0.0f, 0.0f, 1.0f};
                                    ImGui::PushStyleColor(ImGuiCol_Text, red);
                                    ImGui::Text("%s", selectRotErrorText.c_str());
                                    ImGui::PopStyleColor();
                                }

                                default:
                                    break;
                            }


                            ImGui::EndTable();
                        }
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

                ImGui::Text("Editor");
                ImGui::Separator();

                ImGui::SliderFloat("Block Scale", &blockScale, 1, 5);

                ImGui::Checkbox("Show cursor", &cursor.show);

                ImGui::Text("Player Mode");
                ImGui::Separator();

                if(ImGui::SliderFloat("Player speed", &player.speed, 0.01, 1))
                {
                }

                ImGui::SliderFloat("Player gravity", &player.gravitySpeed, -0.05, -1);

                if(ImGui::SliderFloat("Player height", &player.height, 0.25, 5))
                {

                }

                ImGui::EndTabItem();
            }
               
            flags = tabState == TabState::DEBUG_TAB ? ImGuiTabItemFlags_SetSelected : 0;
            if(ImGui::BeginTabItem("Debug", nullptr, flags))
            {
                if(ImGui::IsItemActive())
                    tabState = TabState::DEBUG_TAB;

                ImGui::Text("Camera info");
                ImGui::Separator();

                auto cameraRot = camera.GetRotation();                
                ImGui::SliderFloat2("Rotation", &cameraRot.x, 0.0f, glm::two_pi<float>(), "%.3f", ImGuiSliderFlags_NoInput);
                auto cameraPos = camera.GetPos();
                ImGui::InputFloat3("Global Position", &cameraPos.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
                auto chunkPos = Game::Map::ToChunkIndex(cameraPos, blockScale);
                ImGui::InputInt3("Chunk", &chunkPos.x, ImGuiInputTextFlags_ReadOnly);
                auto blockPos = Game::Map::ToGlobalPos(cameraPos, blockScale);
                ImGui::InputInt3("Block", &blockPos.x, ImGuiInputTextFlags_ReadOnly);

                ImGui::Text("Cursor Info");
                ImGui::Separator();

                auto cursorChunk = Game::Map::ToChunkIndex(cursor.pos);
                ImGui::InputInt3("Cursor Chunk Location", &cursorChunk.x, ImGuiInputTextFlags_ReadOnly);
                auto cursorBlock = cursor.pos;
                ImGui::InputInt3("Cursor Block Location", &cursorBlock.x, ImGuiInputTextFlags_ReadOnly);

                auto pointedChunk = Game::Map::ToChunkIndex(pointedBlockPos);
                ImGui::InputInt3("Pointed Chunk Location", &pointedChunk.x, ImGuiInputTextFlags_ReadOnly);
                auto pointedBlock = pointedBlockPos;
                ImGui::InputInt3("Pointed Block Location", &pointedBlock.x, ImGuiInputTextFlags_ReadOnly);

                auto hittingBlock = intersecting;
                ImGui::Checkbox("Intersecting", &hittingBlock);
                
                
                ImGui::Text("Debug Options");
#ifdef _DEBUG
                ImGui::Separator();
                ImGui::Checkbox("New map system", &newMapSys);
                ImGui::SameLine();
                ImGui::Checkbox("Intersection Optimization", &optimizeIntersection);
                ImGui::SameLine();
#endif
                ImGui::Checkbox("Draw Chunk borders", &drawChunkBorders);


                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }

    ImGui::End();

    #ifdef _DEBUG
    if(showDemo)
        ImGui::ShowDemoWindow(&showDemo);
    #endif

    // Draw GUI
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

