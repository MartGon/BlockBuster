
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
    class Client : public App::App
    {
    public:
        Client(::App::Configuration config) : App{config} {}

        void Start() override;
        void Update() override;
        bool Quit() override;
    private:
        GL::Shader shader = GL::Shader::FromFolder(SHADERS_DIR, "vertex.glsl", "fragment.glsl");
        Rendering::Mesh cube;
        Rendering::Mesh slope;

        GL::Texture texture = GL::Texture::FromFolder(TEXTURES_DIR, "SmoothStone.png");
        GL::Texture gTexture = GL::Texture::FromFolder(TEXTURES_DIR, "SmoothStone.png");

        bool showDemoWindow = true;
        bool quit = false;
        bool gravity = false;
        bool noclip = false;
        bool moveCamera = false;

        const float scale = 5.0f;
        const float playerScale = 1.0f;
        const float gravitySpeed = -0.4f;

        glm::vec2 mousePos;
        glm::vec3 cameraPos{0.0f, 12.0f, 8.0f};

        std::vector<Game::Block> blocks;
        
        uint frame = 0;
        AppGame::Player player;
    };
}