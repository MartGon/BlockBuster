#include <Editor.h>

#include <iostream>
#include <debug/Debug.h>

#include <array>
#include <cstring>

// #### GUI - PopUps #### \\

void BlockBuster::Editor::Editor::EditTextPopUp(const EditTextPopUpParams& params)
{
    auto displaySize = io_->DisplaySize;
    ImGui::SetNextWindowPos(ImVec2{displaySize.x * 0.5f, displaySize.y * 0.5f}, ImGuiCond_Always, ImVec2{0.5f, 0.5f});
    bool onPopUp = state == params.popUpState;
    if(ImGui::BeginPopupModal(params.name.c_str(), &onPopUp, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
    {   
        bool accept = ImGui::InputText("File name", params.textBuffer, params.bufferSize, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CharsNoBlank);
        bool empty = std::strlen(params.textBuffer) <= 0;

        Util::Result<bool> res = Util::CreateSuccess<bool>(true);
        if((ImGui::Button("Accept") || accept) && !empty)
        {
            res = params.onAccept();

            if(res.type == Util::ResultType::SUCCESS)
            {
                params.onError = false;
                
                ClosePopUp();
            }
            else
            {
                params.onError = true;
                params.errorText = params.errorPrefix + params.textBuffer + ": " + res.err.info + '\n';
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
    else if(state == params.popUpState)
    {
        ClosePopUp();
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
    auto onAccept = [this](){this->SaveProject(); return Util::CreateSuccess<bool>(true);};
    auto onCancel = [](){};
    EditTextPopUpParams params{PopUpState::SAVE_AS, "Save As", fileName, 32, onAccept, onCancel, errorPrefix, onError, errorText};
    EditTextPopUp(params);
}

void BlockBuster::Editor::Editor::LoadTexturePopUp()
{
    std::string errorPrefix = "Could not open texture ";
    auto onAccept = [this](){
            auto res = this->LoadTexture();
            return res;
    };
    auto onCancel = [](){};
    EditTextPopUpParams params{PopUpState::LOAD_TEXTURE, "Load Texture", textureFilename, 32, onAccept, onCancel, errorPrefix, onError, errorText};
    EditTextPopUp(params);
}

static std::vector<SDL_DisplayMode> GetDisplayModes()
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

static std::string DisplayModeToString(int w, int h, int rr)
{
    return std::to_string(w) + " x " + std::to_string(h) + " " + std::to_string(rr) + " Hz";
}

static std::string DisplayModeToString(SDL_DisplayMode mode)
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
            ClosePopUp();

            if(std::strlen(fileName) > 0)
            {
                SaveProject();
                onWarningExit();
            }
            else
                OpenPopUp(PopUpState::SAVE_AS);
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
            glm::vec3 cameraPos = (glm::vec3)this->goToPos * this->project.map.GetBlockScale();
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

void BlockBuster::Editor::Editor::SetTextureFolderPopUp()
{
    std::string errorPrefix = "Could not set texture folder ";
    auto onAccept = [this](){
        if(std::filesystem::is_directory(this->textureFolderPath))
        {
            this->project.map.textureFolder = this->textureFolderPath;
            
            return Util::CreateSuccess<bool>(true);
        }
        else
            return Util::CreateError<bool>("Path is not a folder");
    };
    auto onCancel = [](){};
    EditTextPopUpParams params{PopUpState::SET_TEXTURE_FOLDER, "Set Texture Folder", textureFolderPath, 128, onAccept, 
        onCancel, errorPrefix, onError, errorText};
    EditTextPopUp(params);
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
        this->goToPos = Game::Map::ToGlobalPos(camera.GetPos(), this->project.map.GetBlockScale());
    };
    popUps[GO_TO_BLOCK].update = std::bind(&BlockBuster::Editor::Editor::GoToBlockPopUp, this);

    popUps[SET_TEXTURE_FOLDER].name = "Set Texture Folder";
    popUps[SET_TEXTURE_FOLDER].onOpen = [this](){
        std::strcpy(this->textureFolderPath, this->project.map.textureFolder.string().c_str());
    };
    popUps[SET_TEXTURE_FOLDER].update = std::bind(&BlockBuster::Editor::Editor::SetTextureFolderPopUp, this);
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

// #### Widgets #### \\

void BlockBuster::Editor::Editor::HelpMarker(const char* text)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(text);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
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

        if(ImGui::BeginMenu("Project", true))
        {
            
            if(ImGui::MenuItem("Set Texture Folder"))
            {
                OpenPopUp(PopUpState::SET_TEXTURE_FOLDER);
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

        if(ImGui::BeginMenu("Help", true))
        {
            if(ImGui::MenuItem("Keyboard Shortcut Reference", "Ctrl + Shift + K"))
            {
                showShortcutWindow = true;
            }

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

// #### GUI - Misc #### \\

void BlockBuster::Editor::Editor::SyncGUITextures()
{
    auto& tPalette = project.map.tPalette;
    auto tCount = tPalette.GetCount();

    guiTextures.clear();
    guiTextures.reserve(tCount);
    for(unsigned int i = 0; i < tCount; i++)
    {
        auto res = tPalette.GetMember(i);
        auto handle = tPalette.GetTextureArray()->GetHandle();
        ImGui::Impl::ExtraData ea;
        ea.array = ImGui::Impl::TextureArrayData{res.data.id};
        auto texture = ImGui::Impl::Texture{handle, ImGui::Impl::TextureType::TEXTURE_ARRAY, ea};
        guiTextures.push_back(texture);
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

// #### GUI - Help #### \\

void BlockBuster::Editor::Editor::HelpShortCutWindow()
{
    if(!showShortcutWindow)
        return;

    auto& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
    if(ImGui::Begin("Keyboard Shortcuts Reference", &showShortcutWindow))
    {
        if(ImGui::BeginTable("## Shortcuts", 4))
        {
            ImGui::TableSetupColumn("##Camera", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("##Edit", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("##Navigation", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("##Test Mode", ImGuiTableColumnFlags_WidthFixed);

            ImGui::TableNextColumn();
            ImGui::TableHeader("Camera");
            ImGui::TableNextColumn();
            ImGui::TableHeader("Edit");
            ImGui::TableNextColumn();
            ImGui::TableHeader("Navigation");
            ImGui::TableNextColumn();
            ImGui::TableHeader("Test Mode");
            
            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::Text("Camera movement");
            ImGui::BulletText("W - Forward");
            ImGui::BulletText("S - Backward");
            ImGui::BulletText("A - Strafe left");
            ImGui::BulletText("D - Strafe right");
            ImGui::BulletText("Q - Ascend");
            ImGui::BulletText("E - Descend");
            

            ImGui::Text("Camera rotation");
            ImGui::BulletText("Up Arrow - Pitch up");
            ImGui::BulletText("Down Arrow - Pitch down");
            ImGui::BulletText("Left Arrow - Turn left");
            ImGui::BulletText("Right Arrow - Turn right");
            

            ImGui::Text("Camera mode");
            ImGui::SameLine();
            HelpMarker("Editor Camera: Movement ignores the camera rotation. Can only rotate camera with arrows\n"
                    "FPS Camera: Movement according to camera angle. Can rotate camera with mouse");
            ImGui::BulletText("Middle mouse button (hold) - Set FPS camera");
            ImGui::BulletText("F - Toggle FPS/Editor camera");

            ImGui::TableNextColumn();

            ImGui::Text("Tool selection");
            ImGui::BulletText("Ctrl + 1 - Place block");
            ImGui::BulletText("Ctrl + 2 - Rotate block");
            ImGui::BulletText("Ctrl + 3 - Paint block");
            ImGui::BulletText("Ctrl + 4 - Select blocks");
            

            ImGui::Text("Place block tool");
            ImGui::BulletText("1 - Select block type");
            ImGui::BulletText("2 - Select slope type");
            

            ImGui::Text("Rotate block tool");
            ImGui::BulletText("1 - Select y axis");
            ImGui::BulletText("2 - Select z axis");
            

            ImGui::Text("Paint block tool");
            ImGui::BulletText("1 - Select texture display");
            ImGui::BulletText("2 - Select color display");
            
            ImGui::Text("Select blocks tool");
            ImGui::Text("Movement");
            ImGui::BulletText("Numpad 8 - Move cursor on +Z");
            ImGui::BulletText("Numpad 2 - Move cursor on -Z");
            ImGui::BulletText("Numpad 6 - Move cursor on +X");
            ImGui::BulletText("Numpad 4 - Move cursor on -X");
            ImGui::BulletText("Numpad 7 - Move cursor on +Y");
            ImGui::BulletText("Numpad 9 - Move cursor on -Y");
            ImGui::Text("Scaling");
            ImGui::BulletText("Ctrl + Numpad 8 - Scale cursor on +Z");
            ImGui::BulletText("Ctrl + Numpad 2 - Scale cursor on -Z");
            ImGui::BulletText("Ctrl + Numpad 6 - Scale cursor on +X");
            ImGui::BulletText("Ctrl + Numpad 4 - Scale cursor on -X");
            ImGui::BulletText("Ctrl + Numpad 7 - Scale cursor on +Y");
            ImGui::BulletText("Ctrl + Numpad 9 - Scale cursor on -Y");
            
            ImGui::TableNextColumn();

            ImGui::Text("Tab navigation");
            ImGui::BulletText("Alt + 1 - Tools Tab");
            ImGui::BulletText("Alt + 2 - Options Tab");
            ImGui::BulletText("Alt + 3 - Debug Tab");

            ImGui::Text("File/Edit/Options");
            ImGui::BulletText("Check the menus");

            ImGui::TableNextColumn();
            ImGui::Text("Mode");
            ImGui::BulletText("P - Toggle Editor/Test Mode");

            ImGui::Text("Player Movement");
            ImGui::BulletText("W - Forward");
            ImGui::BulletText("S - Backward");
            ImGui::BulletText("A - Strafe left");
            ImGui::BulletText("D - Strafe right");

            ImGui::Text("Player Rotation");
            ImGui::BulletText("Mouse Up - Pitch up");
            ImGui::BulletText("Mouse Down - Pitch down");
            ImGui::BulletText("Mouse Left - Turn left");
            ImGui::BulletText("Mouse Right - Turn right");
            

            ImGui::EndTable();
        }
        ImGui::End();
    }
}

// #### GUI - Tools #### \\

void BlockBuster::Editor::Editor::SelectBlockTypeGUI()
{
    ImGui::Text("Block Type");
    ImGui::SameLine();
    ImGui::RadioButton("Block", &blockType, Game::BlockType::BLOCK);
    ImGui::SameLine();
    ImGui::RadioButton("Slope", &blockType, Game::BlockType::SLOPE);
}

void BlockBuster::Editor::Editor::SelectBlockDisplayGUI()
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
    int columns = glm::max(1, glm::min((int)(region.x / effectiveSize.x), MAX_COLUMNS));
    int entries = displayType == Game::DisplayType::TEXTURE ? guiTextures.size() : project.map.cPalette.GetCount();
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
                ImGui::Image(&guiTextures[i], iconSize);
            }
            else if(displayType == Game::DisplayType::COLOR)
            {
                auto color = Rendering::Uint8ColorToFloat(project.map.cPalette.GetMember(i).data.color);
                ImGui::ColorButton("## color", color);
            }
        }       
        ImGui::TableNextRow();
        
        ImGui::EndTable();
    }

    if(displayType == Game::DisplayType::COLOR)
    {
        auto color = Rendering::Uint8ColorToFloat(project.map.cPalette.GetMember(colorId).data.color);
        if(ImGui::ColorButton("Chosen Color", color))
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
                auto color = Rendering::FloatColorToUint8(colorPick);
                if(!project.map.cPalette.HasColor(color))
                {
                    auto res = project.map.cPalette.AddColor(color);
                    colorId = res.data.id;
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
        ImGui::SameLine();
        HelpMarker("Warning!: Every texture must be a square and have the same size and format (RGB/RGBA)");
    }
}

// #### GUI - Base #### \\

void BlockBuster::Editor::Editor::GUI()
{
    // Clear GUI buffer
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(window_);
    ImGui::NewFrame();

    HelpShortCutWindow();

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
                        SelectBlockTypeGUI();
                    }

                    if(pbSelected || paintSelected)
                    {
                        SelectBlockDisplayGUI();
                    }
                    
                    if(rotbSelected)            
                    {
                        ImGui::Text("Axis");

                        ImGui::SameLine();
                        ImGui::RadioButton("Y", &axis,Game::RotationAxis::Y);
                        ImGui::SameLine();
                        ImGui::RadioButton("Z", &axis,Game::RotationAxis::Z);
                    }

                    if(selectSelected)
                    {
                        if(ImGui::BeginTable("##Select Scale", 4))
                        {
                            ImGui::TableSetupColumn("##Title", ImGuiTableColumnFlags_WidthFixed, 60);
                            ImGui::TableSetupColumn("##Scale x", ImGuiTableColumnFlags_WidthFixed, 180);
                            ImGui::TableSetupColumn("##Scale y", ImGuiTableColumnFlags_WidthFixed, 180);
                            ImGui::TableSetupColumn("##Scale z", ImGuiTableColumnFlags_WidthFixed, 180);
                            ImGui::TableNextColumn();

                            ImGui::Text("Scale");
                            ImGui::TableNextColumn();
                            
                            auto flag = movingSelection ? ImGuiInputTextFlags_ReadOnly : 0;
                            ImGui::InputInt("X", &cursor.scale.x, 1, 1, flag);
                            ImGui::TableNextColumn();
                            ImGui::InputInt("Y", &cursor.scale.y, 1, 1, flag);
                            ImGui::TableNextColumn();
                            ImGui::InputInt("Z", &cursor.scale.z, 1, 1, flag);

                            cursor.scale = glm::max(cursor.scale, glm::ivec3{1, 1, 1});

                            ImGui::EndTable();
                        }

                        if(ImGui::BeginTable("##Select Pos", 4))
                        {
                            ImGui::TableSetupColumn("##Title", ImGuiTableColumnFlags_WidthFixed, 60);
                            ImGui::TableSetupColumn("##Pos x", ImGuiTableColumnFlags_WidthFixed, 180);
                            ImGui::TableSetupColumn("##Pos y", ImGuiTableColumnFlags_WidthFixed, 180);
                            ImGui::TableSetupColumn("##Pos z", ImGuiTableColumnFlags_WidthFixed, 180);
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
                        
                        if(ImGui::CollapsingHeader("Select Sub-tools"))
                        {
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
                                    const std::array<const char*, 4> names = std::array<const char*, 4>{"Move", "Edit", "Rotate", "Fill"};
                                    for(unsigned int i = SelectSubTool::MOVE; i < SelectSubTool::END; i++)
                                    {
                                        auto subTool = static_cast<SelectSubTool>(i);
                                        ImGui::TableNextColumn();
                                        ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_SelectableTextAlign, {0.5, 0});
                                        if(ImGui::Selectable(names[i], &selectTool, subTool))
                                            OnChooseSelectSubTool(selectTool);

                                        ImGui::PopStyleVar();
                                    }
                                    

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
                                        if(ImGui::BeginTable("## Rotate or Mirror", 2))
                                        {
                                            ImGui::TableSetupColumn("## whatever", ImGuiTableColumnFlags_WidthFixed);
                                            ImGui::TableNextColumn();
                                            
                                            ImGui::TableHeader("Rotate");
                                            ImGui::TableNextColumn();

                                            ImGui::TableHeader("Mirror");
                                            ImGui::TableNextRow();
                                            ImGui::TableNextColumn();

                                            ImGui::Text("Axis");

                                            ImGui::SameLine();
                                            ImGui::RadioButton("X", &selectRotAxis,Game::RotationAxis::X);
                                            ImGui::SameLine();
                                            ImGui::RadioButton("Y", &selectRotAxis,Game::RotationAxis::Y);
                                            ImGui::SameLine();
                                            ImGui::RadioButton("Z", &selectRotAxis,Game::RotationAxis::Z);

                                            ImGui::Text("Rotation angle (deg)");

                                            ImGui::SameLine();
                                            ImGui::RadioButton("90", &selectRotType, Game::RotType::ROT_90);
                                            ImGui::SameLine();
                                            ImGui::RadioButton("180", &selectRotType, Game::RotType::ROT_180);

                                            if(ImGui::Button("Rotate selection"))
                                            {
                                                auto res = RotateSelection(selectRotAxis, selectRotType);
                                                selectRotErrorText = res.info;
                                            }

                                            ImGui::TableNextColumn();

                                            ImGui::Text("Plane");
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


                                            if(ImGui::Button("Mirror selection"))
                                            {
                                                auto res = MirrorSelection(selectMirrorPlane);
                                                selectRotErrorText = res.info;
                                            }

                                            ImGui::EndTable();
                                        }
                                        // Error text
                                        ImVec4 red{1.0f, 0.0f, 0.0f, 1.0f};
                                        ImGui::PushStyleColor(ImGuiCol_Text, red);
                                        ImGui::Text("%s", selectRotErrorText.c_str());
                                        ImGui::PopStyleColor();
                                        break;
                                    }

                                    case SelectSubTool::FILL_OR_PAINT:
                                    {
                                        if(ImGui::Button("Fill"))
                                        {
                                            FillSelection();
                                        }

                                        ImGui::SameLine();
                                        if(ImGui::Button("Replace"))
                                        {
                                            ReplaceSelection();
                                        }

                                        ImGui::SameLine();
                                        if(ImGui::Button("Paint"))
                                        {
                                            PaintSelection();
                                        }

                                        SelectBlockTypeGUI();
                                        SelectBlockDisplayGUI();
                                    }

                                    default:
                                        break;
                                }


                                ImGui::EndTable();
                            }
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

                if(ImGui::SliderFloat("Block Scale", &blockScale, 1, 5))
                {
                    project.map.SetBlockScale(blockScale);
                };

                ImGui::Text("Select Tool - Cursor Display");
                ImGui::SameLine();
                ImGui::RadioButton("Single Block", &cursor.mode, CursorMode::SCALED);
                ImGui::SameLine();
                ImGui::RadioButton("Multiple Blocks", &cursor.mode, CursorMode::BLOCKS);

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

                if(ImGui::CollapsingHeader("Camera info"))
                {
                    auto cameraRot = camera.GetRotation();                
                    ImGui::SliderFloat2("Rotation", &cameraRot.x, 0.0f, glm::two_pi<float>(), "%.3f", ImGuiSliderFlags_NoInput);
                    auto cameraPos = camera.GetPos();
                    ImGui::InputFloat3("Global Position", &cameraPos.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
                    auto chunkPos = Game::Map::ToChunkIndex(cameraPos, blockScale);
                    ImGui::InputInt3("Chunk", &chunkPos.x, ImGuiInputTextFlags_ReadOnly);
                    auto blockPos = Game::Map::ToGlobalPos(cameraPos, blockScale);
                    ImGui::InputInt3("Block", &blockPos.x, ImGuiInputTextFlags_ReadOnly);
                }
                if(ImGui::CollapsingHeader("Cursor Info"))
                {
                    auto cursorChunk = Game::Map::ToChunkIndex(cursor.pos);
                    ImGui::InputInt3("Cursor Chunk Location", &cursorChunk.x, ImGuiInputTextFlags_ReadOnly);
                    auto cursorBlock = cursor.pos;
                    ImGui::InputInt3("Cursor Block Location", &cursorBlock.x, ImGuiInputTextFlags_ReadOnly);

                    auto pointedChunk = Game::Map::ToChunkIndex(pointedBlockPos);
                    ImGui::InputInt3("Pointed Chunk Location", &pointedChunk.x, ImGuiInputTextFlags_ReadOnly);
                    auto pointedBlockLoc = pointedBlockPos;
                    ImGui::InputInt3("Pointed Block Location", &pointedBlockLoc.x, ImGuiInputTextFlags_ReadOnly);

                    auto hittingBlock = intersecting;
                    ImGui::Checkbox("Intersecting", &hittingBlock);

                    glm::vec3 blockRot{0.0f};
                    int type = -1;
                    if(intersecting)
                    {
                        auto pointedBlock = project.map.GetBlock(pointedBlockPos);
                        blockRot = pointedBlock.GetRotation();
                        type = pointedBlock.type;
                    }
                    ImGui::InputInt("Block Type", &type, 1, 100, ImGuiInputTextFlags_ReadOnly);
                    ImGui::InputFloat3("Block Rotation", &blockRot.x, "%.2f", ImGuiInputTextFlags_ReadOnly);
                
                }
                if(ImGui::CollapsingHeader("Debug Options"))
                {
                #ifdef _DEBUG
                    ImGui::Separator();
                    ImGui::Checkbox("New map system", &newMapSys);
                    ImGui::SameLine();
                    ImGui::Checkbox("Intersection Optimization", &optimizeIntersection);
                    ImGui::SameLine();
                    ImGui::Checkbox("Use Texture Array", &useTextureArray);
                    ImGui::SameLine();
                #endif
                    ImGui::Checkbox("Draw Chunk borders", &drawChunkBorders);
                }

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
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData(), gui_vao.GetHandle());
    auto windowSize = GetWindowSize();
    int scissor_box[4] = { 0, 0, windowSize.x, windowSize.y };
    ImGui_ImplOpenGL3_RestoreState(scissor_box);
}
