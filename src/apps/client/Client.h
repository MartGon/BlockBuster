
#include <app/App.h>

#include <gl/Shader.h>
#include <gl/VertexArray.h>
#include <gl/Texture.h>

#include <rendering/Camera.h>
#include <rendering/Mesh.h>
#include <rendering/Primitive.h>
#include <rendering/Rendering.h>

#include <game/PlayerController.h>
#include <game/Map.h>
#include <game/CameraController.h>
#include <game/ChunkMeshMgr.h>

#include <entity/Player.h>

#include <networking/enetw/ENetW.h>

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

        // Input
        void HandleSDLEvents();

        // Rendering
        void DrawScene();

        // Networking
        void UpdateNetworking();
        void SendPlayerMovement();

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
        std::unordered_map<Entity::ID, Entity::Player> playerTable;
        float PLAYER_SPEED = 2.f;

        // Networking
        ENet::Host host;
        ENet::PeerId serverId = 0;
        uint8_t playerId = 0;
        uint32_t tickCount = 0;

        // App
        bool quit = false;
    };
}