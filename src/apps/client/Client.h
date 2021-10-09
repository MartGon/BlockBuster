
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

#include <networking/Networking.h>

namespace BlockBuster
{
    class Client : public App::App
    {
    public:
        Client(::App::Configuration config);

        void Start() override;
        void Update() override;
        bool Quit() override;
        
    private:

        void DoUpdate(double deltaTime);
        void HandleSDLEvents();
        void UpdateNetworking();
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
        int drawMode = GL_FILL;

        // Update
        const double UPDATE_RATE = 0.020;

        // Controls
        ::App::Client::CameraController camController_;

        // Player transforms
        std::unordered_map<uint8_t, Math::Transform> players;
        std::unordered_map<uint8_t, glm::vec3> playerTargets;
        float PLAYER_SPEED = 2.f;

        // Networking
        ENet::Host host;

        // App
        bool quit = false;
    };
}