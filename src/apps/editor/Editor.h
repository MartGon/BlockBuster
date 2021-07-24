
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
        bool Quit() override;
        
    private:

        // Rendering
        Rendering::Mesh& GetMesh(Game::BlockType type);

        // World
        Game::Block* GetBlock(glm::vec3 pos);

        // Editor
        void UpdateCamera();
        void UseTool(glm::vec<2, int> mousePos, bool rightButton = false);

        // GUI
        void SaveAsPopUp();
        void MenuBar();
        void GUI();

        // Rendering
        GL::Shader shader = GL::Shader::FromFolder(SHADERS_DIR, "vertex.glsl", "fragment.glsl");
        GL::Texture texture = GL::Texture::FromFolder(TEXTURES_DIR, "SmoothStone.png");
        Rendering::Mesh cube;
        Rendering::Mesh slope;
        Rendering::Camera camera;
        const float CAMERA_MOVE_SPEED = 0.25f;
        const float CAMERA_ROT_SPEED = glm::radians(1.0f);

        // World
        std::vector<Game::Block> blocks;
        const float BLOCK_SCALE = 2.0f;
        
        // Editor
        bool quit = false;

        enum Tool
        {
            PLACE_BLOCK,

            ROTATE_BLOCK
        };

        enum class RotationAxis
        {
            X,
            Y,
            Z
        };
        Tool tool = PLACE_BLOCK;
        Game::BlockType blockType = Game::BlockType::BLOCK;
        RotationAxis axis = RotationAxis::Y;

        // GUI
        enum class PopUpState
        {
            NONE,
            SAVE_AS
        };

        // Tool - Place
        ImVec4 displayColor;
        Game::DisplayType displayType = Game::DisplayType::TEXTURE;
        
        // Edit
        bool mouseCamera = false;

        // Save
        PopUpState state = PopUpState::NONE;
        char fileName[16] = "Map.bbm";
    };
}