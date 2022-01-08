#pragma once

#include <GameState/GameState.h>

#include <gl/Shader.h>
#include <gl/VertexArray.h>
#include <gl/Texture.h>

#include <rendering/Camera.h>
#include <rendering/Mesh.h>
#include <rendering/Model.h>
#include <rendering/Primitive.h>
#include <rendering/Rendering.h>

#include <game/PlayerController.h>
#include <game/Map.h>
#include <game/CameraController.h>
#include <game/ChunkMeshMgr.h>
#include <game/PlayerAvatar.h>
#include <game/FPSAvatar.h>

#include <util/BBTime.h>
#include <util/Ring.h>

#include <entity/Player.h>

#include <networking/enetw/ENetW.h>
#include <networking/Command.h>
#include <networking/Snapshot.h>

namespace BlockBuster
{
    class InGame : public GameState
    {
    public:
        InGame(Client* client);
        
        void Start() override;
        void Update() override;

    private:

        void DoUpdate(Util::Time::Seconds deltaTime);

        // Input
        void HandleSDLEvents();

        // Networking
        void RecvServerSnapshots();
        void UpdateNetworking();

        // Networking - Prediction
        void PredictPlayerMovement(Networking::Command::User::PlayerMovement cmd, uint32_t cmdId);
        void SmoothPlayerMovement();
        glm::vec3 PredPlayerPos(glm::vec3 pos, glm::vec3 moveDir, Util::Time::Seconds deltaTime);

        // Networking - Entity Interpolation
        Util::Time::Seconds GetCurrentTime();
        Util::Time::Seconds GetRenderTime();
        Util::Time::Seconds TickToTime(uint32_t tick);
        void EntityInterpolation();
        void EntityInterpolation(Entity::ID playerId, const Networking::Snapshot& a, const Networking::Snapshot& b, float alpha);

        // Rendering
        void DrawScene();
        void DrawGUI();
        void Render();

        // GameState
        std::unique_ptr<GameState> gameState;

        // Scene
        ::App::Client::Map map_;

        // Rendering
        GL::Shader shader;
        GL::Shader chunkShader;
        Rendering::Mesh cylinder;
        Rendering::Mesh sphere;
        Rendering::Mesh cube;
        Rendering::Mesh slope;
        Game::PlayerAvatar playerAvatar;
        Game::FPSAvatar fpsAvatar;
        Rendering::Camera camera_;
        int drawMode = GL_FILL;

        Util::Time::SteadyPoint preSimulationTime;
        Util::Time::Seconds simulationLag;
        Util::Time::Seconds deltaTime;
        Util::Time::Seconds minFrameInterval;
        double maxFPS = 60.0;

        // GUI
        GL::VertexArray guiVao;

        // Controls
        ::App::Client::CameraController camController_;

        // Player transforms
        std::unordered_map<Entity::ID, Entity::Player> playerTable;
        std::unordered_map<Entity::ID, Entity::Player> prevPlayerPos;
        float PLAYER_SPEED = 5.f;

        //TODO: Move this to its own class. 
        // Player Model
        uint32_t modelId = -1;
        float sliderPrecision = 2.0f;
        glm::vec3 modelOffset{0.0f};
        glm::vec3 modelScale{1.0f};
        glm::vec3 modelRot{0.0f};

        // Networking
        ENet::Host host;
        ENet::PeerId serverId = 0;
        uint8_t playerId = 0;
        Util::Time::Seconds serverTickRate{0.0};
        bool connected = false;
        const uint32_t redundantInputs = 3;
        Util::Ring<Networking::Snapshot> snapshotHistory{16};

        // Networking - Prediction
        struct Prediction
        {
            Networking::Command::User::PlayerMovement cmd;
            glm::vec3 origin;
            glm::vec3 dest;
            Util::Time::SteadyPoint time;
        };
        Util::Ring<Prediction> predictionHistory_{128};
        uint32_t cmdId = 0;
        uint32_t lastAck = 0;
        Util::Time::Seconds predOffset{0};

        const Util::Time::Seconds ERROR_CORRECTION_DURATION{3.0};
        glm::vec3 errorCorrectionDiff{0};
        Util::Time::SteadyPoint errorCorrectionStart;

        // Networking - Entity Interpolation
        const Util::Time::Seconds EXTRAPOLATION_DURATION{0.25};
        Networking::Snapshot extrapolatedSnapshot;
        Util::Time::Seconds offsetTime{0};
    };
}