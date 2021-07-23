
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

        void GUI();
        void UpdateCamera();
        void UseTool(glm::vec<2, int> mousePos);

        GL::Shader shader = GL::Shader::FromFolder(SHADERS_DIR, "vertex.glsl", "fragment.glsl");
        GL::Texture texture = GL::Texture::FromFolder(TEXTURES_DIR, "SmoothStone.png");
        Rendering::Mesh cube;
        Rendering::Mesh slope;
        Rendering::Camera camera;

        std::vector<Game::Block> blocks;
        const float BLOCK_SCALE = 2.0f;
        const float CAMERA_MOVE_SPEED = 0.25f;
        const float CAMERA_ROT_SPEED = glm::radians(1.0f);

        bool quit = false;

        // GUI
        enum Tool
        {
            PLACE_BLOCK,
            REMOVE_BLOCK
        };
        Tool tool = PLACE_BLOCK;
        Game::BlockType blockType = Game::BlockType::BLOCK;

        ImVec4 color;
    };
}