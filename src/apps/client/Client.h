
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
#include <game/CameraController.h>

#include <enet/enet.h>

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

        void HandleSDLEvents();

        GL::Shader shader = GL::Shader::FromFolder(SHADERS_DIR, "circleVertex.glsl", "circleFrag.glsl");
        Rendering::Mesh circle;
        Rendering::Mesh sphere;
        Rendering::Mesh cylinder;
        Rendering::Camera camera_;
        Game::App::CameraController camController_;
        
        bool quit = false;
        int drawMode = GL_FILL;
    };
}