#pragma once

#include <EditorFwd.h>

#include <VideoSettingsPopUp.h>
#include <gui/PopUpMgr.h>

namespace BlockBuster::Editor
{
    class Editor;

    class EditorGUI
    {
    friend class Editor;

    public:
        EditorGUI(Editor& editor);

        enum PopUpState
        {
            SAVE_AS,
            OPEN_MAP,
            LOAD_TEXTURE,
            UNSAVED_WARNING,
            VIDEO_SETTINGS,
            GO_TO_BLOCK,
            SET_TEXTURE_FOLDER,
            ARE_YOU_SURE,
            MAX
        };

        enum TabState
        {
            TOOLS_TAB,
            OPTIONS_TAB,

            DEBUG_TAB
        };

        void OpenMapPopUp();
        void SaveAsPopUp();
        void LoadTexturePopUp();
        void VideoOptionsPopUp();
        void UnsavedWarningPopUp();
        void GoToBlockPopUp();
        void SetTextureFolderPopUp();
        void OpenWarningPopUp(std::function<void()> onExit);

        void InitPopUps();
        void OpenPopUp(PopUpState puState);
        void ClosePopUp();
        bool IsAnyPopUpOpen();

        // GUI - Misc
        void SyncGUITextures();

        // Menu - File
        void MenuBar();
        void MenuNewMap();
        void MenuOpenMap();
        void MenuSave();
        void MenuSaveAs();

        // GUI - Help
        void HelpShortCutWindow();

        // Tools - GUI
        void ToolsTab();
        void PlaceBlockGUI(Game::Block& block, const char* tableId, ImVec2 tableSize = ImVec2{0, 0});
        void SelectBlockTypeGUI(Game::Block& block);
        void SelectBlockDisplayGUI(Game::Block& block);
        void RotateBlockGUI();
        void SelectBlocksGUI();
        void PlaceObjectGUI();

        // GameObject
        void PropertyInput(Entity::GameObject* go, const char* key, Entity::GameObject::Property::Type type);

        void ToolOptionsGUI();
        void GUI();

    private:
        Editor* editor = nullptr;

        GL::VertexArray gui_vao;
        const int MAX_TEXTURES = 32;
        std::vector<ImGui::Impl::Texture> guiTextures;

        GUI::PopUpMgr<PopUpState::MAX> puMgr;
        App::VideoSettingsPopUp videoSettingsPopUp;
        TabState tabState = TabState::TOOLS_TAB;
        bool pickingColor = false;
        bool newColor = false;
        bool disablePaintButtons = false;

        bool onError = false;
        std::string errorText;
        std::string selectRotErrorText;
        std::function<void()> onWarningExit;

        // PopUp Inputs
        char fileName[16] = "";
        char textureFilename[32] = "texture.png";
        char textureFolderPath[128] = "";
        glm::ivec3 goToPos{0};
        Game::Block placeBlock{Game::BlockType::BLOCK, Game::BlockRot{}, Game::Display{Game::DisplayType::COLOR, 0}};;

        // Find tool
        Game::Block findBlock{Game::BlockType::BLOCK, Game::BlockRot{}, Game::Display{Game::DisplayType::COLOR, 0}};

        Game::RotationAxis axis = Game::RotationAxis::Y;
        Game::RotationAxis selectRotAxis = Game::RotationAxis::Y;
        MirrorPlane selectMirrorPlane = MirrorPlane::XY;
        Game::RotType selectRotType = Game::RotType::ROT_90;
        glm::ivec3 selectedObj{0};

        glm::vec4 colorPick;

        bool showShortcutWindow = false;
        bool drawChunkBorders = false;
        #ifdef _DEBUG
            uint32_t modelId = 2;
            float sliderPrecision = 5.0f;
            glm::vec3 modelOffset{0.0f};
            glm::vec3 modelRot{0.0f};
            glm::vec3 modelScale{1.0f};
            bool showDemo = false;
            bool newMapSys = true;
            bool optimizeIntersection = true;
            bool useTextureArray = true;
        #endif
    };
}