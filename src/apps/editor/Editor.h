
#include <app/App.h>

#include <gl/Shader.h>
#include <gl/VertexArray.h>
#include <gl/Texture.h>

#include <rendering/Camera.h>
#include <rendering/Mesh.h>
#include <rendering/Primitive.h>
#include <rendering/Rendering.h>

#include <game/Player.h>
#include <game/Block.h>

#include <ToolAction.h>

#include <functional>

namespace BlockBuster
{
    namespace Editor
    {
        class Editor : public App::App
        {
        public:
            Editor(::App::Configuration config) : App{config} {}

            void Start() override;
            void Update() override;
            void Shutdown() override;
            bool Quit() override;
            
        private:

            // Enums
            enum class PopUpState
            {
                NONE,
                SAVE_AS,
                OPEN_MAP,
                LOAD_TEXTURE,
                UNSAVED_WARNING,
                VIDEO_SETTINGS,
            };

            enum TabState
            {
                TOOLS_TAB,
                OPTIONS_TAB
            };

            enum class CameraMode
            {
                EDITOR = 0,
                FPS
            };

            // Rendering
            Rendering::Mesh& GetMesh(Game::BlockType type);
            glm::vec4 GetBorderColor(glm::vec4 basecolor, glm::vec4 darkColor = glm::vec4{0.0f, 0.0f, 0.0f, 1.0f}, glm::vec4 lightColor = glm::vec4{1.0f});
            bool LoadTexture();
            bool IsTextureInPalette(std::filesystem::path folder, std::filesystem::path textureName);

            // World
            Game::Block* GetBlock(glm::vec3 pos);
            void ResizeBlocks();
            void NewMap();
            void SaveMap();
            bool OpenMap();

            // Editor
            void UpdateEditor();

            void UpdateEditorCamera();
            void UpdateFPSCameraPosition();
            void UpdateFPSCameraRotation(SDL_MouseMotionEvent motion);
            void SetCameraMode(CameraMode cameraMode);

            void UseTool(glm::vec<2, int> mousePos, ActionType actionType = ActionType::LEFT_BUTTON);
            void QueueAction(std::unique_ptr<ToolAction> action);
            void DoToolAction(std::unique_ptr<ToolAction> action);
            void DoToolAction();
            void UndoToolAction();
            void ClearActionHistory();
            
            void HandleKeyShortCut(const SDL_KeyboardEvent& e);

            Game::Display GetBlockDisplay();
            void SetBlockDisplay(Game::Display display);

            void SetUnsaved(bool unsaved);
            void OpenWarningPopUp(std::function<void()> onExit);
            void Exit();

            // Test Mode
            void UpdatePlayerMode();

            // Options
            void HandleWindowEvent(SDL_WindowEvent winEvent);
            void ApplyVideoOptions(::App::Configuration::WindowConfig& config);
            std::string GetConfigOption(const std::string& key, std::string defaultValue = "");

            // GUI - PopUps
            void OpenMapPopUp();
            void SaveAsPopUp();
            void LoadTexturePopUp();
            void VideoOptionsPopUp();
            void UnsavedWarningPopUp();
            struct BasicPopUpParams
            {
                PopUpState popUpState;
                std::string name;
                char* textBuffer; 
                size_t bufferSize;
                std::function<bool()> onAccept;
                std::function<void()> onCancel;
                std::string errorPrefix;

                bool& onError;
                std::string& errorText;
            };
            void EditTextPopUp(const BasicPopUpParams& params);

            // GUI
            void MenuBar();

            // Menu - File
            void MenuNewMap();
            void MenuOpenMap();
            void MenuSave();
            void MenuSaveAs();

            void GUI();

            // Rendering
            GL::Shader shader = GL::Shader::FromFolder(SHADERS_DIR, "vertex.glsl", "fragment.glsl");
            Rendering::Mesh cube;
            Rendering::Mesh slope;
            Rendering::Camera camera;
            CameraMode cameraMode = CameraMode::EDITOR;
            const float CAMERA_MOVE_SPEED = 0.25f;
            const float CAMERA_ROT_SPEED = glm::radians(1.0f);

            // World
            std::vector<Game::Block> blocks;
            
            // Editor
            bool quit = false;
            bool unsaved = true;
            std::function<void()> onWarningExit;

            // Tools
            enum Tool
            {
                PLACE_BLOCK,
                ROTATE_BLOCK,
                PAINT_BLOCK
            };
            Tool tool = PLACE_BLOCK;
            unsigned int actionIndex = 0;
            std::vector<std::unique_ptr<BlockBuster::Editor::ToolAction>> actionHistory;

            // Tool - Place
            Game::BlockType blockType = Game::BlockType::BLOCK;

            // Tool - Rotate
            enum class RotationAxis
            {
                X,
                Y,
                Z
            };
            RotationAxis axis = RotationAxis::Y;

            // Tool - Paint
            Game::DisplayType displayType = Game::DisplayType::COLOR;
            int textureId = 0;
            int colorId = 0;

            std::filesystem::path textureFolder = TEXTURES_DIR;
            char textureFilename[32] = "texture.png";

            const int MAX_TEXTURES = 32;
            std::vector<GL::Texture> textures;
            std::vector<glm::vec4> colors;

            bool pickingColor = false;
            glm::vec4 colorPick;

            const glm::vec4 yellow = glm::vec4{1.0f, 1.0f, 0.0f, 1.0f};
            const glm::vec4 darkBlue = glm::vec4{20.f / 255.f, 0.0f, 0.5f, 1.0f};

            int preColorBlockIndex = 0;

            // Tools - Cursor
            struct Cursor{
                bool show = true;
                bool enabled = false;
                Math::Transform transform;
                glm::vec4 color;
                Game::BlockType type;
            };
            Cursor cursor;

            // GUI
            PopUpState state = PopUpState::NONE;
            TabState tabState = TabState::TOOLS_TAB;
            bool onError = false;
            std::string errorText;

            // File
            std::filesystem::path mapsFolder = ".";
            char fileName[16] = "Map.bbm";
            const int magicNumber = 0xB010F0;
            bool newMap = true;

            // Config
            ::App::Configuration::WindowConfig preConfig;

            // Options
            float blockScale = 2.0f;

            // Debug
            bool showDemo = false;

            // Test
            bool playerMode = false;
            AppGame::Player player;
        };
    }
}