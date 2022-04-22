#include <Editor.h>
#include <EditorGUI.h>

#include <iostream>
#include <debug/Debug.h>

#include <imgui_internal.h>

#include <array>
#include <cstring>
#include <regex>

// #### GUI - PopUps #### \\

using namespace BlockBuster::Editor;
using namespace BlockBuster;
using namespace Entity;

EditorGUI::EditorGUI(Editor& editor) : editor{&editor}, videoSettingsPopUp{editor}
{

}

void EditorGUI::OpenWarningPopUp(std::function<void()> onExit)
{
    puMgr.Open(UNSAVED_WARNING);
    onWarningExit = onExit;
}

void EditorGUI::InitPopUps()
{
    GUI::EditTextPopUp saveAs;
    saveAs.SetTitle("Save As");
    saveAs.SetStringSize(16);
    saveAs.SetOnOpen([this](){
        auto sa = puMgr.GetAs<GUI::EditTextPopUp>(SAVE_AS);
        sa->SetPlaceHolder(this->fileName);
    });
    saveAs.SetOnAccept([this](std::string map){
        auto sa = puMgr.GetAs<GUI::EditTextPopUp>(SAVE_AS);

        std::regex validName{"[A-z0-9]*"};
        std::smatch m;
        bool isValid = std::regex_match(map, m, validName);
        if(!isValid)
            sa->SetErrorText("Map name is not valid");
        else
        {
            this->fileName = map;
            this->editor->SaveProject();
        }

        return isValid;
    });
    puMgr.Set(SAVE_AS, std::make_unique<GUI::EditTextPopUp>(saveAs));

    GUI::EditTextPopUp openMap;
    openMap.SetTitle("Open Map");
    openMap.SetStringSize(16);
    openMap.SetErrorText("Could not open map");
    openMap.SetOnOpen([this](){
        auto om = puMgr.GetAs<GUI::EditTextPopUp>(OPEN_MAP);
        om->SetPlaceHolder(this->fileName);
    });
    openMap.SetOnAccept([this](std::string map){
         this->fileName = map;
        auto res = this->editor->OpenProject(); return res.IsSuccess();
    });
    puMgr.Set(OPEN_MAP, std::make_unique<GUI::EditTextPopUp>(openMap));

    GUI::EditTextPopUp loadTexture;
    loadTexture.SetTitle("Load Texture");
    loadTexture.SetPlaceHolder("texture.png");
    loadTexture.SetErrorText("Could not load texture");
    loadTexture.SetStringSize(32);
    loadTexture.SetOnAccept([this](std::string path){
        std::strcpy(textureFilename, path.data());
        auto res = this->editor->LoadTexture();
        if(!res.IsSuccess())
        {
            auto stf = puMgr.GetAs<GUI::EditTextPopUp>(LOAD_TEXTURE);
            stf->SetErrorText("Could load texture: " + std::string(res.err.info));
        }
        return res.IsSuccess();
    });
    puMgr.Set(LOAD_TEXTURE, std::make_unique<GUI::EditTextPopUp>(loadTexture));

    GUI::GenericPopUp unsavedWarning;
    unsavedWarning.SetTitle("Unsaved Content");
    unsavedWarning.SetOnDraw([this](){
        ImGui::Text("There's unsaved content.\nWhat would you like to do?");

        if(ImGui::Button("Save"))
        {
            this->puMgr.Close();

            if(fileName.size() > 0)
            {
                editor->SaveProject();
                onWarningExit();
            }
            else
                OpenPopUp(PopUpState::SAVE_AS);
        }
        ImGui::SameLine();

        if(ImGui::Button("Don't Save"))
        {
            editor->unsaved = false;
            this->puMgr.Close();

            this->onWarningExit();
        }
        ImGui::SameLine();

        if(ImGui::Button("Cancel"))
        {
            this->puMgr.Close();
        }
    });
    puMgr.Set(UNSAVED_WARNING, std::make_unique<GUI::GenericPopUp>(unsavedWarning));

    App::VideoSettingsPopUp videoPopUp{*this->editor};
    puMgr.Set(VIDEO_SETTINGS, std::make_unique<App::VideoSettingsPopUp>(videoPopUp));

    GUI::GenericPopUp goToBlock;
    goToBlock.SetTitle("Go to block");
    goToBlock.SetOnOpen([this](){
        this->goToPos = Game::Map::ToGlobalPos(editor->camera.GetPos(), editor->project.map.GetBlockScale());
    });
    goToBlock.SetOnDraw([this](){
        ImGui::InputInt3("Position", &goToPos.x);

        if(ImGui::Button("Accept"))
        {
            glm::vec3 cameraPos = Game::Map::ToRealPos(goToPos, editor->project.map.GetBlockScale());
            editor->camera.SetPos(cameraPos);
            puMgr.Close();
        }

        ImGui::SameLine();
        if(ImGui::Button("Cancel"))
        {
            puMgr.Close();
        }
    });
    puMgr.Set(GO_TO_BLOCK, std::make_unique<GUI::GenericPopUp>(goToBlock));

    GUI::BasicPopUp basicPopUp;
    basicPopUp.SetButtonVisible(true);
    basicPopUp.SetTitle("Warning!");
    basicPopUp.SetText("Are you sure?\nThis action cannot be undone");
    basicPopUp.SetCloseable(true);

    puMgr.Set(ARE_YOU_SURE, std::make_unique<GUI::BasicPopUp>(basicPopUp));
}

void EditorGUI::OpenPopUp(PopUpState state)
{
    puMgr.Open(state);
}

void EditorGUI::ClosePopUp()
{
    puMgr.Close();
}

bool EditorGUI::IsAnyPopUpOpen()
{
    return puMgr.IsOpen();
}

// #### GUI - Misc #### \\

void EditorGUI::SyncGUITextures()
{
    auto& tPalette = editor->project.map.tPalette;
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

void EditorGUI::MenuBar()
{
    // Pop Ups
    puMgr.Update();

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
                editor->UndoToolAction();
            }

            if(ImGui::MenuItem("Redo", "Ctrl + Shift + Z"))
            {
                editor->DoToolAction();
            }

            ImGui::Separator();

            if(ImGui::MenuItem("Go to Block", "Ctrl + G"))
            {
                puMgr.Open(PopUpState::GO_TO_BLOCK);
            }

            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("Settings", true))
        {
            if(ImGui::MenuItem("Video", "Ctrl + Shift + G"))
            {
                puMgr.Open(PopUpState::VIDEO_SETTINGS);
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

void EditorGUI::MenuNewMap()
{
    if(editor->unsaved)
    {
        auto onExit = [this](){editor->NewProject();};
        OpenWarningPopUp(onExit);
    }
    else
        editor->NewProject();
}

void EditorGUI::MenuOpenMap()
{
    if(editor->unsaved)
    {
        auto onExit = [this](){OpenPopUp(PopUpState::OPEN_MAP);};
        OpenWarningPopUp(onExit);
    }
    else
        OpenPopUp(PopUpState::OPEN_MAP);
}

void EditorGUI::MenuSave()
{
    if(editor->newMap)
        OpenPopUp(PopUpState::SAVE_AS);
    else
        editor->SaveProject();
}

void EditorGUI::MenuSaveAs()
{
    OpenPopUp(PopUpState::SAVE_AS);
}

// #### GUI - Help #### \\

void EditorGUI::HelpShortCutWindow()
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
            GUI::HelpMarker("Editor Camera: Movement ignores the camera rotation. Can only rotate camera with arrows\n"
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

void EditorGUI::ToolsTab()
{
    if(ImGui::IsItemActive())
        tabState = TabState::TOOLS_TAB;

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
        
        // Map Tools Table
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        ImGui::SetCursorPosX(4);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1, 1, 0.0f, 1.0f});
        ImGui::Text("Map Tools");
        ImGui::PopStyleColor();
        if(ImGui::BeginTable("##Map Tools", 2, 0, ImVec2{120, 0}))
        {
            ImGui::TableNextColumn();
            ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_SelectableTextAlign, {0.5, 0});
            if(ImGui::Selectable("Place", editor->tool == Editor::PLACE_BLOCK, 0, ImVec2{0, 0}))
            {
                editor->SelectTool(Editor::PLACE_BLOCK);
            }

            ImGui::TableNextColumn();
            if(ImGui::Selectable("Rotate", editor->tool == Editor::ROTATE_BLOCK, 0, ImVec2{0, 0}))
            {
                editor->SelectTool(Editor::ROTATE_BLOCK);
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            if(ImGui::Selectable("Paint", editor->tool == Editor::PAINT_BLOCK, 0, ImVec2{0, 0}))
            {
                editor->SelectTool(Editor::PAINT_BLOCK);
            }

            ImGui::TableNextColumn();
            if(ImGui::Selectable("Select", editor->tool == Editor::SELECT_BLOCKS, 0, ImVec2{0, 0}))
            {
                editor->SelectTool(Editor::SELECT_BLOCKS);
            }

            ImGui::PopStyleVar();

            ImGui::EndTable();
        }

        ImGui::SetCursorPosX(4);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1, 1, 0.0f, 1.0f});
        ImGui::Text("Object Tools");
        ImGui::PopStyleColor();
        if(ImGui::BeginTable("##Object Tools", 2, 0, ImVec2{120, 0}))
        {
            ImGui::TableNextColumn();
            ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_SelectableTextAlign, {0.5, 0});
            if(ImGui::Selectable("Place", editor->tool == Editor::PLACE_OBJECT, 0, ImVec2{0, 0}))
            {
                editor->SelectTool(Editor::PLACE_OBJECT);
            }

            ImGui::TableNextColumn();
            if(ImGui::Selectable("Select", editor->tool == Editor::SELECT_OBJECT, 0, ImVec2{0, 0}))
            {
                editor->SelectTool(Editor::SELECT_OBJECT);
            }
            ImGui::PopStyleVar();

            ImGui::EndTable();
        }
        

        // Tools Options
        ImGui::TableNextColumn();

        ToolOptionsGUI();

        ImGui::EndTable();
    }
}

void EditorGUI::PlaceBlockGUI(Game::Block& block, const char* tableId, ImVec2 tableSize)
{
    if(ImGui::BeginTable(tableId, 2, 0, tableSize))
    {
        ImGui::TableSetupColumn("##Block Type", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("##Painting Options", 0);

        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
        ImGui::TableSetColumnIndex(0);
        ImGui::TableHeader("##Block Type");
        ImGui::SameLine();
        ImGui::Text("Block Type");

        ImGui::TableSetColumnIndex(1);
        ImGui::TableHeader("##Painting Options");
        ImGui::SameLine();
        ImGui::Text("Painting Options");

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
            SelectBlockTypeGUI(block);
        ImGui::TableNextColumn();
            SelectBlockDisplayGUI(block);

        ImGui::EndTable();
    }
}

int FindIndex(Game::RotType target)
{
    for(int i = Game::ROT_0 ; i < Game::RotType::ROT_MAX; i++)
        if(i == target)
            return i;

    return 0;
}

void EditorGUI::SelectBlockTypeGUI(Game::Block& block)
{
    auto& blockType = block.type;

    ImGui::RadioButton("Block", &blockType, Game::BlockType::BLOCK);
    ImGui::SameLine();
    ImGui::RadioButton("Slope", &blockType, Game::BlockType::SLOPE);

    const char* rotTypes[Game::RotType::ROT_MAX] = {"0", "90", "180", "270"};
    auto index = FindIndex(placeBlock.rot.y);
    if(ImGui::Combo("Y rot", &index, rotTypes, Game::RotType::ROT_MAX))
        placeBlock.rot.y = static_cast<Game::RotType>(index);
    index = FindIndex(placeBlock.rot.z);
    if(ImGui::Combo("Z rot", &index, rotTypes, Game::RotType::ROT_MAX))
        placeBlock.rot.z = static_cast<Game::RotType>(index);
}

void EditorGUI::SelectBlockDisplayGUI(Game::Block& block)
{
    auto& displayType = block.display.type;
    auto& colorId = block.display.id;
    auto& textureId = block.display.id;

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
    int entries = displayType == Game::DisplayType::TEXTURE ? guiTextures.size() : editor->project.map.cPalette.GetCount();
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
                auto pCol = editor->project.map.cPalette.GetMember(i).data.color;
                auto color = Rendering::Uint8ColorToFloat(pCol);
                ImGui::ColorButton("## color", color);
            }
        }       
        ImGui::TableNextRow();
        
        ImGui::EndTable();
    }

    if(disablePaintButtons)
        ImGui::PushDisabled();

    if(displayType == Game::DisplayType::COLOR)
    {
        auto pCol = editor->project.map.cPalette.GetMember(colorId).data.color;
        auto color = Rendering::Uint8ColorToFloat(pCol);
        if(ImGui::Button("New Color"))
        {
            ImGui::OpenPopup("Color Picker");
            pickingColor = true;
            newColor = true;
        }
        ImGui::SameLine();
        if(ImGui::ColorButton("Chosen Color", color))
        {
            ImGui::OpenPopup("Color Picker");
            pickingColor = true;
        }
        ImGui::SameLine();
        ImGui::Text("Edit color");
        if(ImGui::BeginPopup("Color Picker"))
        {
            ImGui::ColorPicker4("##picker", &colorPick.x, ImGuiColorEditFlags__OptionsDefault);
            ImGui::EndPopup();
        }
        else if(pickingColor)
        {
            pickingColor = false;
            if(displayType == Game::DisplayType::COLOR)
            {
                auto color = Rendering::FloatColorToUint8(colorPick);
                if(!editor->project.map.cPalette.HasColor(color))
                {
                    if(newColor)
                    {
                        auto res = editor->project.map.cPalette.AddColor(color);
                        colorId = res.data.id;
                    }
                    else
                        editor->project.map.cPalette.SetColor(colorId, color);
                }
            }

            newColor = false;
        }
    }
    else if(displayType == Game::DisplayType::TEXTURE)
    {
        if(editor->newMap)
            ImGui::PushDisabled();

        if(ImGui::Button("Add Texture"))
        {
            OpenPopUp(PopUpState::LOAD_TEXTURE);
        }
        std::string tip = "Info: You must place the texture file in: "+ editor->mapMgr.GetMapFolder(fileName).string() + 
        "/textures/ \n\nWarning!: Every texture must be a square and have the same size and format (RGB/RGBA)";
        GUI::AddToolTip(tip.c_str());

        if(editor->newMap)
            ImGui::PopDisabled();
        
        if(editor->newMap)
        {
            ImGui::SameLine();
            GUI::HelpMarker("You need to save the project first before adding a Texture");
        }

        auto isEmpty = editor->project.map.tPalette.GetCount() == 0;
        if(isEmpty)
            ImGui::PushDisabled();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{1, 0, 0, 1});
        ImGui::SameLine();
        if(ImGui::Button("Reset Textures"))
        {
            auto ays = puMgr.GetAs<GUI::BasicPopUp>(ARE_YOU_SURE);
            ays->SetButtonCallback([this](){
                editor->ResetTexturePalette();    
            });
            puMgr.Open(ARE_YOU_SURE);
        }
        ImGui::PopStyleColor();
        GUI::AddToolTip("Warning!: This will remove all textures from all the blocks, so you can load a texture with new format or size");
        if(isEmpty)
            ImGui::PopDisabled();

        auto texA = editor->project.map.tPalette.GetTextureArray();
        ImGui::Text("Texture Data");
        ImGui::Text("Texture Size: %ix%i px", texA->GetTextureSize(), texA->GetTextureSize());
        ImGui::SameLine();
        ImGui::Text("Color Channels: %i", texA->GetChannels());
    }

    if(disablePaintButtons)
        ImGui::PopDisabled();
}

void EditorGUI::RotateBlockGUI()
{
    ImGui::Text("Axis");

    ImGui::SameLine();
    ImGui::RadioButton("Y", &axis,Game::RotationAxis::Y);
    ImGui::SameLine();
    ImGui::RadioButton("Z", &axis,Game::RotationAxis::Z);
}

void EditorGUI::SelectBlocksGUI()
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
        
        auto flag = editor->movingSelection ? ImGuiInputTextFlags_ReadOnly : 0;
        ImGui::InputInt("X", &editor->cursor.scale.x, 1, 1, flag);
        ImGui::TableNextColumn();
        ImGui::InputInt("Y", &editor->cursor.scale.y, 1, 1, flag);
        ImGui::TableNextColumn();
        ImGui::InputInt("Z", &editor->cursor.scale.z, 1, 1, flag);

        editor->cursor.scale = glm::max(editor->cursor.scale, glm::ivec3{1, 1, 1});

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

        auto nextPos = editor->cursor.pos;
        bool hasSelectionMoved = false;
        hasSelectionMoved |= ImGui::InputInt("X", &nextPos.x, 1);
        ImGui::TableNextColumn();
        hasSelectionMoved |= ImGui::InputInt("Y", &nextPos.y, 1);
        ImGui::TableNextColumn();
        hasSelectionMoved |= ImGui::InputInt("Z", &nextPos.z, 1);
        ImGui::TableNextColumn();
        if(hasSelectionMoved)
            editor->MoveSelectionCursor(nextPos);

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
            const std::array<const char*, 4> names = std::array<const char*, 4>{"Move", "Edit", "Rotate", "Fill"};
            for(unsigned int i = Editor::SelectSubTool::MOVE; i < Editor::SelectSubTool::END; i++)
            {
                auto subTool = static_cast<Editor::SelectSubTool>(i);
                ImGui::TableNextColumn();
                ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_SelectableTextAlign, {0.5, 0});
                if(ImGui::Selectable(names[i], &editor->selectTool, subTool))
                    editor->OnChooseSelectSubTool(editor->selectTool);

                ImGui::PopStyleVar();
            }
            

            ImGui::EndTable();
        }

        ImGui::TableNextColumn();
        switch (editor->selectTool)
        {
            case Editor::SelectSubTool::MOVE:
            {
                const char* label = editor->movingSelection ? "Stop Moving" : "Start Moving";
                ImVec4 color = editor->movingSelection ? ImVec4{1.0f, 0.0f, 0.0f, 1.0f} : ImVec4{0.0f, 1.0f, 0.0f, 1.0f};
                ImGui::PushStyleColor(ImGuiCol_Button, color);
                if(ImGui::Button(label))
                {
                    editor->movingSelection = !editor->movingSelection;
                    if(editor->movingSelection)
                        editor->SelectBlocks();
                    else
                        editor->ClearSelection();
                }
                ImGui::PopStyleColor();

                #ifdef _DEBUG
                ImGui::Text("Selected %zu blocks", editor->selection.size());
                ImVec2 size {0, 40};
                if(ImGui::BeginTable("Selection", 1, ImGuiTableFlags_Hideable | ImGuiTableFlags_ScrollY, size))
                {
                    for(auto pair : editor->selection)
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
            
            case Editor::SelectSubTool::EDIT:
            {
                if(ImGui::Button("Copy"))
                {
                    editor->CopySelection();
                }
                ImGui::SameLine();

                if(ImGui::Button("Cut"))
                {
                    editor->CutSelection();
                }
                ImGui::SameLine();

                if(ImGui::Button("Paste"))
                {
                    editor->PasteSelection();
                }
                ImGui::SameLine();

                if(ImGui::Button("Remove"))
                {
                    editor->RemoveSelection();
                }
                ImGui::SameLine();
                break;
            }

            case Editor::SelectSubTool::ROTATE_OR_MIRROR:
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
                        auto res = editor->RotateSelection(selectRotAxis, selectRotType);
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
                        auto res = editor->MirrorSelection(selectMirrorPlane);
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

            case Editor::SelectSubTool::FILL_OR_PAINT:
            {
                // Check table id
                auto regionAvail = ImVec2{ImGui::GetContentRegionAvail().x / 2, 0};
                
                auto curX = ImGui::GetCursorPosX();
                ImGui::Text("Source");
                ImGui::SameLine();
                ImGui::SetCursorPosX(curX + regionAvail.x);
                ImGui::Text("Target");
                
                PlaceBlockGUI(placeBlock, "##Place Block Options Table", regionAvail);
                ImGui::SameLine();

                disablePaintButtons = true;
                PlaceBlockGUI(findBlock, "##Find Block Options Table", regionAvail);
                disablePaintButtons = false;

                ImGui::Text("In selection:");
                ImGui::SameLine();
                if(ImGui::Button("Replace all"))
                {
                    editor->ReplaceAllInSelection();
                }
                GUI::AddToolTip("Replaces every block (including empty ones) in selection with an instance of the source block");

                ImGui::SameLine();
                if(ImGui::Button("Replace any"))
                {
                    editor->ReplaceAnyInSelection();
                }
                GUI::AddToolTip("Replaces blocks in selection that are not empty, with an instance of the source block");

                ImGui::SameLine();
                if(ImGui::Button("Replace target"))
                {
                    editor->ReplaceInSelection();
                }
                GUI::AddToolTip("Replaces blocks in selection, which match with target, with an instance of the source block");
                
                ImGui::SameLine();
                ImGui::Text("In map:");

                ImGui::SameLine();
                if(ImGui::Button("Map replace all"))
                {
                    editor->ReplaceAll(placeBlock, findBlock);
                }
                GUI::AddToolTip("Replaces blocks in map, which match with target, with an instance of the source block");

                ImGui::Text("Misc.:");
                
                ImGui::SameLine();
                if(ImGui::Button("Fill Empty"))
                {
                    editor->FillEmptyInSelection();
                }
                GUI::AddToolTip("Fills empty blocks in selection with an instance of the source block");

                ImGui::SameLine();
                if(ImGui::Button("Paint"))
                {
                    editor->PaintSelection();
                }
                GUI::AddToolTip("Paints blocks in selection that are not empty, with the selected painting");
            }

            default:
                break;
        }


        ImGui::EndTable();
    }
}

void EditorGUI::PlaceObjectGUI()
{
    if(ImGui::Combo("Object Type", (int*)&editor->placedGo.type, Entity::GameObject::objectTypesToString, Entity::GameObject::COUNT))
    {
        editor->placedGo = GameObject::Create(static_cast<Entity::GameObject::Type>(editor->placedGo.type));
    }

    ImGui::Dummy(ImVec2{10, 0}); ImGui::SameLine();
    ImGui::Text("Object Properties");
    auto propertiesTemplate = GameObject::GetPropertyTemplate(editor->placedGo.type);
    for(auto p : propertiesTemplate)
    {
        PropertyInput(&editor->placedGo, p.name.c_str(), p.type);
    }
}

void EditorGUI::PropertyInput(Entity::GameObject* go, const char* key, GameObject::Property::Type type)
{
    switch(type)
    {
        case GameObject::Property::Type::BOOL:
        {
            auto& ref = go->properties[key];
            ImGui::Checkbox(key, &std::get<bool>(ref.value));
            break;
        }
        case GameObject::Property::Type::FLOAT:
        {
            auto& ref = go->properties[key];
            ImGui::InputFloat(key, &std::get<float>(ref.value));
            break;
        }
        case GameObject::Property::Type::INT:
        {
            auto& ref = go->properties[key];
            ImGui::InputInt(key, &std::get<int>(ref.value));
            break;
        }
        case GameObject::Property::Type::STRING:
        {
            auto& ref = go->properties[key];
            auto& str = std::get<std::string>(ref.value);
            ImGui::InputText(key, str.data(), 16);
            break;
        }

        default:
            break;
    }
}

// #### GUI - Base #### \\

void EditorGUI::ToolOptionsGUI()
{
    switch (editor->tool)
    {
    case Editor::PLACE_BLOCK:
        PlaceBlockGUI(placeBlock, "##Place Block Options Table");
        break;

    case Editor::PAINT_BLOCK:
        SelectBlockDisplayGUI(placeBlock);
        break;

    case Editor::ROTATE_BLOCK:
        RotateBlockGUI();
        break;

    case Editor::SELECT_BLOCKS:
        SelectBlocksGUI();
        break;
    
    case Editor::PLACE_OBJECT:
        PlaceObjectGUI();
        break;

    case Editor::SELECT_OBJECT:
        if(editor->project.map.GetMap()->GetGameObject(selectedObj))
        {
            PlaceObjectGUI();
            if(ImGui::Button("Apply Changes"))
                editor->EditGameObject();
        }
        else
            ImGui::Text("No object selected. Click in an object to select it");
        break;
    
    default:
        break;
    }
}

void EditorGUI::GUI()
{
    // Clear GUI buffer
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(editor->window_);
    ImGui::NewFrame();

    HelpShortCutWindow();

    bool open;
    auto displaySize = editor->io_->DisplaySize;
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
                ToolsTab();

                ImGui::EndTabItem();
            }

            flags = tabState == TabState::OPTIONS_TAB ? ImGuiTabItemFlags_SetSelected : 0;
            if(ImGui::BeginTabItem("Options", nullptr, flags))
            {
                if(ImGui::IsItemActive())
                    tabState = TabState::OPTIONS_TAB;

                if(ImGui::CollapsingHeader("Editor"))
                {
                    ImGui::Text("Select Tool - Cursor Display");
                    ImGui::SameLine();
                    ImGui::RadioButton("Single Block", &editor->cursor.mode, Editor::CursorMode::SCALED);
                    ImGui::SameLine();
                    ImGui::RadioButton("Multiple Blocks", &editor->cursor.mode, Editor::CursorMode::BLOCKS);
                    ImGui::Checkbox("Show cursor (Place/Rotate/Paint Tools)", &editor->cursor.show);
                }
                if(ImGui::CollapsingHeader("Camera"))
                {
                    ImGui::SliderFloat("Movement speed", &editor->cameraController.moveSpeed, 0.05f, 2.f);
                    ImGui::SliderFloat("Rotation speed", &editor->cameraController.rotSpeed, glm::radians(0.25f), glm::radians(4.0f));
                }
                if(ImGui::CollapsingHeader("Player Mode"))
                {
                    ImGui::SliderFloat("Player speed", &editor->playerController.speed, 0.01, 1);
                    ImGui::SliderFloat("Player jump speed", &editor->playerController.jumpSpeed, 0.0f, 50.0f);
                    ImGui::SliderFloat("Gravity Acceleration", &editor->playerController.gravityAcceleration, -50.0f, 0.0f);
                    //ImGui::SliderFloat("Player height", &player.height, 0.25, 5);
                }
                /*
                if(ImGui::SliderFloat("Block Scale", &blockScale, 1, 5))
                {
                    project.map.SetBlockScale(blockScale);
                };
                */

                ImGui::EndTabItem();
            }
               
            flags = tabState == TabState::DEBUG_TAB ? ImGuiTabItemFlags_SetSelected : 0;
            if(ImGui::BeginTabItem("Debug", nullptr, flags))
            {
                if(ImGui::IsItemActive())
                    tabState = TabState::DEBUG_TAB;

                if(ImGui::CollapsingHeader("Camera info"))
                {
                    auto cameraRot = editor->camera.GetRotation();                
                    ImGui::SliderFloat2("Rotation", &cameraRot.x, 0.0f, glm::two_pi<float>(), "%.3f", ImGuiSliderFlags_NoInput);
                    auto cameraPos = editor->camera.GetPos();
                    ImGui::InputFloat3("Global Position", &cameraPos.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
                    auto chunkPos = Game::Map::ToChunkIndex(cameraPos, editor->project.map.GetBlockScale());
                    ImGui::InputInt3("Chunk", &chunkPos.x, ImGuiInputTextFlags_ReadOnly);
                    auto blockPos = Game::Map::ToGlobalPos(cameraPos, editor->project.map.GetBlockScale());
                    ImGui::InputInt3("Block", &blockPos.x, ImGuiInputTextFlags_ReadOnly);
                }
                if(ImGui::CollapsingHeader("Cursor Info"))
                {
                    auto cursorChunk = Game::Map::ToChunkIndex(editor->cursor.pos);
                    ImGui::InputInt3("Cursor Chunk Location", &cursorChunk.x, ImGuiInputTextFlags_ReadOnly);
                    auto cursorBlock = editor->cursor.pos;
                    ImGui::InputInt3("Cursor Block Location", &cursorBlock.x, ImGuiInputTextFlags_ReadOnly);

                    auto pointedChunk = Game::Map::ToChunkIndex(editor->pointedBlockPos);
                    ImGui::InputInt3("Pointed Chunk Location", &pointedChunk.x, ImGuiInputTextFlags_ReadOnly);
                    auto pointedBlockLoc = editor->pointedBlockPos;
                    ImGui::InputInt3("Pointed Block Location", &pointedBlockLoc.x, ImGuiInputTextFlags_ReadOnly);

                    auto hittingBlock = editor->intersecting;
                    ImGui::Checkbox("Intersecting", &hittingBlock);

                    glm::vec3 blockRot{0.0f};
                    int type = -1;
                    if(editor->intersecting)
                    {
                        auto pointedBlock = editor->project.map.GetBlock(editor->pointedBlockPos);
                        blockRot = pointedBlock.GetRotation();
                        type = pointedBlock.type;
                    }
                    ImGui::InputInt("Block Type", &type, 1, 100, ImGuiInputTextFlags_ReadOnly);
                    ImGui::InputFloat3("Block Rotation", &blockRot.x, "%.2f", ImGuiInputTextFlags_ReadOnly);
                
                }
                if(ImGui::CollapsingHeader("Debug Options"))
                {
                    ImGui::Checkbox("Draw Chunk borders", &drawChunkBorders);
                #ifdef _DEBUG
                    ImGui::Separator();
                    ImGui::Checkbox("New map system", &newMapSys);
                    ImGui::SameLine();
                    ImGui::Checkbox("Intersection Optimization", &optimizeIntersection);
                    ImGui::SameLine();
                    ImGui::Checkbox("Use Texture Array", &useTextureArray);
                    
                    auto prevModelId = modelId;
                    if(ImGui::InputInt("ID", (int*)&modelId))
                    {
                        if(auto sm = editor->respawnModel.model->GetSubModel(modelId))
                        {
                            modelOffset = sm->transform.position;
                            modelScale = sm->transform.scale;
                            modelRot = sm->transform.rotation;
                        }
                        else
                            modelId = prevModelId;
                    }
                    ImGui::SliderFloat3("Offset", &modelOffset.x, -sliderPrecision, sliderPrecision);
                    ImGui::SliderFloat3("Scale", &modelScale.x, -sliderPrecision, sliderPrecision);
                    ImGui::SliderFloat3("Rotation", &modelRot.x, -sliderPrecision, sliderPrecision);
                    if(auto sm = editor->respawnModel.model->GetSubModel(modelId))
                    {
                        sm->transform.position = modelOffset;
                        sm->transform.scale = modelScale;
                        sm->transform.rotation = modelRot;
                    }
                    ImGui::InputFloat("Precision", &sliderPrecision);
                #endif
                    
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
    auto windowSize = editor->GetWindowSize();
    int scissor_box[4] = { 0, 0, windowSize.x, windowSize.y };
    ImGui_ImplOpenGL3_RestoreState(scissor_box);
}
