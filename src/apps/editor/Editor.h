
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

namespace BlockBuster
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

        // Rendering
        Rendering::Mesh& GetMesh(Game::BlockType type);
        bool LoadTexture();

        // World
        Game::Block* GetBlock(glm::vec3 pos);
        void NewMap();
        void SaveMap();
        bool LoadMap();

        // Editor
        void UpdateCamera();
        void UseTool(glm::vec<2, int> mousePos, bool rightButton = false);
        Game::Display GetBlockDisplay();
        void SetBlockDisplay(Game::Display display);

        // Options
        void HandleWindowEvent(SDL_WindowEvent winEvent);
        void ApplyVideoOptions(::App::Configuration::WindowConfig& config);
        std::string GetConfigOption(const std::string& key, std::string defaultValue = "");

        // GUI
        void OpenMapPopUp();
        void SaveAsPopUp();
        void LoadTexturePopUp();
        void VideoOptionsPopUp();
        void MenuBar();
        void GUI();

        // Rendering
        GL::Shader shader = GL::Shader::FromFolder(SHADERS_DIR, "vertex.glsl", "fragment.glsl");
        Rendering::Mesh cube;
        Rendering::Mesh slope;
        Rendering::Camera camera;
        const float CAMERA_MOVE_SPEED = 0.25f;
        const float CAMERA_ROT_SPEED = glm::radians(1.0f);

        // World
        std::vector<Game::Block> blocks;
        
        // Editor
        bool quit = false;

        // Tools
        enum Tool
        {
            PLACE_BLOCK,
            ROTATE_BLOCK,
            PAINT_BLOCK
        };
        Tool tool = PLACE_BLOCK;

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
        bool pickingColor = false;
        Game::DisplayType displayType;
        int textureId = 0;
        int colorId = 0;
        std::filesystem::path textureFolder = TEXTURES_DIR;
        const int MAX_TEXTURES = 32;
        std::vector<GL::Texture> textures;
        glm::vec4 colorPick;
        std::vector<glm::vec4> colors;

        // GUI
        enum class PopUpState
        {
            NONE,
            SAVE_AS,
            OPEN_MAP,
            LOAD_TEXTURE,
            VIDEO_SETTINGS,
        };
        PopUpState state = PopUpState::NONE;

        // File
        char fileName[16] = "Map.bbm";
        const int magicNumber = 0xB010F0;
        bool newMap = true;
        std::string errorText;

        // Texture
        char textureFilename[32] = "texture.png";

        // Config
        ::App::Configuration::WindowConfig preConfig;
        
        // Edit
        bool mouseCamera = false;

        // Options
        float blockScale = 2.0f;

        // Debug
        bool showDemo = false;
    };
}