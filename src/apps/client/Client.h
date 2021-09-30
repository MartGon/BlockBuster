
#include <app/App.h>

#include <gl/Shader.h>
#include <gl/VertexArray.h>
#include <gl/Texture.h>

#include <rendering/Camera.h>
#include <rendering/Mesh.h>
#include <rendering/Primitive.h>
#include <rendering/Rendering.h>

#include <client/Player.h>
#include <client/Map.h>
#include <client/CameraController.h>

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

        void DoUpdate(float deltaTime);
        void HandleSDLEvents();
        void DrawScene();

        // Scene
        ::App::Client::Map map_;

        // Rendering
        GL::Shader shader;
        GL::Shader chunkShader;
        Rendering::Mesh cylinder;
        Rendering::Camera camera_;

        // Update
        float deltaTime = 16;

        // Controls
        ::App::Client::CameraController camController_;

        // Player transforms
        std::unordered_map<uint8_t, Math::Transform> players;
        std::unordered_map<uint8_t, glm::vec3> playerTargets;
        
        bool quit = false;
        int drawMode = GL_FILL;
    };
}