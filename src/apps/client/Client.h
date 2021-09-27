
#include <app/App.h>

#include <gl/Shader.h>
#include <gl/VertexArray.h>
#include <gl/Texture.h>

#include <rendering/Camera.h>
#include <rendering/Mesh.h>
#include <rendering/Primitive.h>
#include <rendering/Rendering.h>

#include <client/Player.h>
#include <game/Block.h>
#include <client/CameraController.h>
#include <game/Map.h>

#include <rendering/ChunkMeshMgr.h>

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
        void DrawScene();

        // Scene
        Game::Map::Map map_;

        // Rendering
        GL::Shader shader;
        GL::Shader chunkShader;
        Rendering::Mesh cylinder;
        Rendering::ChunkMesh::Manager chunkMeshMgr_{&map_};
        Rendering::Camera camera_;

        // Controls
        ::App::Client::CameraController camController_;
        
        bool quit = false;
        int drawMode = GL_FILL;
    };
}