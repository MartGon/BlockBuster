
#include <app/App.h>

#include <gl/Shader.h>
#include <gl/VertexArray.h>
#include <gl/Texture.h>

#include <rendering/Camera.h>
#include <rendering/Mesh.h>
#include <rendering/Primitive.h>
#include <rendering/Rendering.h>

#include <game/Player.h>
#include <game/Map.h>
#include <game/CameraController.h>

#include <game/ChunkMeshMgr.h>

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

        void DoUpdate(double deltaTime);
        void HandleSDLEvents();
        void DrawScene();

        // Networking
        void GeneratePlayerTarget(unsigned int playerId);
        void PlayerUpdate(unsigned int playerId, double deltaTime);

        // Time
        double GetCurrentTime() const;

        // Scene
        ::App::Client::Map map_;

        // Rendering
        GL::Shader shader;
        GL::Shader chunkShader;
        Rendering::Mesh cylinder;
        Rendering::Camera camera_;

        // Update
        const double UPDATE_RATE = 0.020;

        // Controls
        ::App::Client::CameraController camController_;

        // Player transforms
        std::unordered_map<uint8_t, Math::Transform> players;
        std::unordered_map<uint8_t, glm::vec3> playerTargets;
        float PLAYER_SPEED = 2.f;
        
        bool quit = false;
        int drawMode = GL_FILL;
    };
}