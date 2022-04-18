
#include <app/App.h>

#include <gl/Shader.h>
#include <gl/VertexArray.h>
#include <gl/Texture.h>
#include <gl/Framebuffer.h>

#include <rendering/Camera.h>
#include <rendering/Mesh.h>
#include <rendering/Primitive.h>
#include <rendering/RenderMgr.h>
#include <rendering/Rendering.h>
#include <rendering/Model.h>
#include <rendering/Skybox.h>

#include <models/Respawn.h>
#include <models/Player.h>
#include <models/ModelMgr.h>

#include <entity/PlayerController.h>
#include <entity/Block.h>
#include <entity/Map.h>
#include <entity/GameObject.h>

#include <game/ChunkMeshMgr.h>
#include <game/CameraController.h>
#include <game/MapMgr.h>

#include <imgui/backends/imgui_impl_opengl3.h>

#include <Project.h>
#include <ToolAction.h>
#include <EditorGUI.h>

#include <functional>

namespace BlockBuster::Editor
{
    using BlockData = std::pair<glm::ivec3, Game::Block>;

    class Editor : public App::AppI
    {
    friend class EditorGUI;

    public:
        Editor(::App::Configuration config) : AppI{config} {}

        void Start() override;
        void Update() override;
        void Shutdown() override;
        bool Quit() override;
        
    private:

        // Enums
        enum class CursorMode
        {
            BLOCKS,
            SCALED
        };

        enum Tool
        {
            PLACE_BLOCK,
            ROTATE_BLOCK,
            PAINT_BLOCK,
            SELECT_BLOCKS,

            PLACE_OBJECT,
            SELECT_OBJECT
        };

        enum SelectSubTool
        {
            MOVE,
            EDIT,
            ROTATE_OR_MIRROR,
            FILL_OR_PAINT,
            END
        };

        struct Result
        {
            bool isOk = true;
            std::string info;
        };

        // Rendering
        Rendering::Mesh& GetMesh(Game::BlockType type);
        glm::vec4 GetBorderColor(glm::vec4 basecolor, glm::vec4 darkColor = glm::vec4{0.0f, 0.0f, 0.0f, 1.0f}, glm::vec4 lightColor = glm::vec4{1.0f});
        Util::Result<bool> LoadTexture();
        bool IsTextureInPalette(std::filesystem::path folder, std::filesystem::path textureName);
        void ResetTexturePalette();
        void Render();

        // World
        void NewProject();
        void SaveProject();
        Util::Result<bool> OpenProject();
        void TakePicture();

        // Editor
        void UpdateEditor();
        void SetCameraMode(::App::Client::CameraMode cameraMode);
        void HandleKeyShortCut(const SDL_KeyboardEvent& e);
        void HandleWindowEvent(SDL_WindowEvent winEvent);
        void RenameMainWindow();
        void SetUnsaved(bool unsaved);
        void Exit();

        // Test Mode
        void UpdatePlayerMode();

        // Tools - General
        void SelectTool(Tool tool);
        void UseTool(glm::vec<2, int> mousePos, ActionType actionType = ActionType::LEFT_BUTTON);
        void QueueAction(std::unique_ptr<ToolAction> action);
        void DoToolAction(std::unique_ptr<ToolAction> action);
        void DoToolAction();
        void UndoToolAction();
        void ClearActionHistory();

        // Paint tool - TODO: Could move this to GUI (Get/Set)
        Game::Display GetBlockDisplay();
        bool IsDisplayValid();
        void SetBlockDisplay(Game::Display display);

        // Selection cursor
        void DrawCursor(Math::Transform t);
        void DrawSelectCursor(glm::ivec3 pos);
        void SetCursorState(bool enabled, glm::ivec3 pos, Game::BlockType blockType, Game::BlockRot rot);
        void MoveSelectionCursor(glm::ivec3 nextPos);

        // Select Tool
        void OnChooseSelectSubTool(SelectSubTool subTool);
        void EnumBlocksInSelection(std::function<void(glm::ivec3 pos, glm::ivec3 offset)> onEach);
        std::vector<BlockData> GetBlocksInSelection(bool globalPos = true);
        void SelectBlocks();
        void ClearSelection();
        bool CanMoveSelection(glm::ivec3 offset);
        bool IsBlockInSelection(glm::ivec3 pos);
        void MoveSelection(glm::ivec3 offset);

        // Select Tool - Edit
        void CopySelection();
        void RemoveSelection();
        void CutSelection();
        void PasteSelection();
        // TODO: Change this to use Util::Result / Oktal::Result
        Result RotateSelection(Game::RotationAxis axis, Game::RotType rotType);
        Result MirrorSelection(MirrorPlane plane);
        void ReplaceAllInSelection();
        void FillEmptyInSelection();
        void ReplaceAnyInSelection();
        void ReplaceInSelection();
        void PaintSelection();
        void ReplaceAll(Game::Block source, Game::Block target);
        
        // Object Tool
        void SelectGameObject(glm::ivec3 pos);
        void EditGameObject();

        // Options
        void ApplyVideoOptions(::App::Configuration::WindowConfig& config) override;

        // Project
        MapMgr mapMgr;
        Project project;

        // Rendering
        Rendering::RenderMgr renderMgr;
        GL::Shader renderShader;
        GL::Shader paintShader;
        GL::Shader chunkShader;
        GL::Shader quadShader;
        GL::Shader skyboxShader;
        GL::Shader billboardShader;

        Rendering::Mesh cube;
        Rendering::Mesh slope;
        Rendering::Mesh cylinder;
        Rendering::Skybox skybox;
        Rendering::Camera camera;
        ::App::Client::CameraController cameraController;

        // Framebuffer;
        GL::Framebuffer framebuffer;
        const glm::ivec2 picSize{800, 600};

        // Models
        Game::Models::ModelMgr modelMgr;
        Game::Models::Player playerAvatar;
        Game::Models::Respawn respawnModel;
        
        // Editor
        std::filesystem::path resourcesDir;
        bool quit = false;
        bool unsaved = true;

        // GUI
        EditorGUI gui{*this};

        // Tools
        Tool tool = PLACE_BLOCK;
        glm::ivec3 pointedBlockPos;
        unsigned int actionIndex = 0;
        std::vector<std::unique_ptr<BlockBuster::Editor::ToolAction>> actionHistory;

        // Tools - Cursor
        struct Cursor{
            bool show = true;
            bool enabled = false;
            glm::ivec3 pos = glm::ivec3{0};
            Game::BlockRot rot {Game::RotType::ROT_0, Game::RotType::ROT_0};
            glm::ivec3 scale{1};
            glm::vec4 color = glm::vec4{1.0f, 1.0f, 0.0f, 1.0f};
            Game::BlockType type = Game::BlockType::BLOCK;
            CursorMode mode = CursorMode::SCALED;
        };
        bool intersecting = false;
        Cursor cursor;

        // Tools - Select
        std::vector<BlockData> selection;
        bool movingSelection = false;
        glm::ivec3 savedPos{0};
        SelectSubTool selectTool = SelectSubTool::MOVE;

        // Tools - Copy/Cut
        std::vector<BlockData> clipboard;

        // Objects
        Entity::GameObject placedGo;

        // File
        std::filesystem::path mapsFolder = ".";
        const int magicNumber = 0xB010F0;
        bool newMap = true;

        // Config
        ::App::Configuration::WindowConfig preConfig;

        // Options
        float blockScale = 2.0f; // TODO: Remove. Stored in map

        // Test
        bool playerMode = false;
        Entity::PlayerController player;
    };
}