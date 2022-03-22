#pragma once

#include <GameState/GameState.h>
#include <GameState/InGame/InGameFwd.h>

#include <gl/Shader.h>
#include <gl/VertexArray.h>
#include <gl/Texture.h>

#include <rendering/Camera.h>
#include <rendering/Mesh.h>
#include <rendering/Model.h>
#include <rendering/Primitive.h>
#include <rendering/Rendering.h>

#include <game/Map.h>
#include <game/CameraController.h>
#include <game/ChunkMeshMgr.h>
#include <game/models/Player.h>
#include <game/models/FPS.h>

#include <util/BBTime.h>
#include <util/Ring.h>

#include <entity/Player.h>
#include <entity/PlayerController.h>

#include <networking/enetw/ENetW.h>
#include <networking/Snapshot.h>
#include <networking/Packets.h>

#include <audio/Audio.h>

#include <GameState/InGame/InGameGUI.h>

namespace BlockBuster
{
    class InGame : public GameState
    {
    friend class InGameGUI;

    public:
        InGame(Client* client, std::string serverDomain, uint16_t serverPort);
        
        void Start() override;
        void Update() override;
        void Shutdown() override;

        void ApplyVideoOptions(App::Configuration::WindowConfig& winConfig) override;

    private:

        void DoUpdate(Util::Time::Seconds deltaTime);

        // Input
        void HandleSDLEvents();

        // Handy
        Entity::Player& GetLocalPlayer();

        // Networking
        // TODO: Preallocate players. This is useful to avoid resizing the vectors/maps
        void OnPlayerJoin(Entity::ID playerId, Networking::PlayerSnapshot playerState);
        void OnPlayerLeave(Entity::ID playerId);
        void OnRecvPacket(ENet::PeerId peerId, uint8_t channelId, ENet::RecvPacket packet);
        void OnRecvPacket(Networking::Packet& packet);
        void RecvServerSnapshots();
        void UpdateNetworking();
        void SendPlayerInput();

        // Networking - Prediction
        using InputReq = Networking::Packets::Client::Input::Req;
        Entity::PlayerController pController;
        void Predict(Entity::PlayerInput playerInput);
        void SmoothPlayerMovement();
        Entity::PlayerState PredPlayerState(Entity::PlayerState a, Entity::PlayerInput playerInput, float playerYaw, Util::Time::Seconds deltaTime);

        // Networking - Entity Interpolation
        Util::Time::Seconds GetCurrentTime();
        Util::Time::Seconds GetRenderTime();
        Util::Time::Seconds TickToTime(uint32_t tick);
        void EntityInterpolation();
        void EntityInterpolation(Entity::ID playerId, const Networking::Snapshot& a, const Networking::Snapshot& b, float alpha);
        glm::vec3 GetLastMoveDir(Entity::ID playerId) const;

        // Rendering
        void DrawScene();
        void DrawCollisionBox(const glm::mat4& viewProjMat, Math::Transform box);
        void DrawGUI();
        void Render();

        // Audio
        void UpdateAudio();

        // Map
        void LoadMap(std::filesystem::path filePath);

        // Game Config 
        void LoadGameOptions();
        void ApplyGameOptions(GameOptions options);
        void WriteGameOptions();

        // Player transforms
        struct PlayerModelState
        {
            Animation::Player shootPlayer;
            Animation::Player idlePlayer;
            Math::Transform armsPivot;
            bool leftFlashActive = false;
            bool rightFlashActive = false;
        };
        std::unordered_map<Entity::ID, Entity::Player> playerTable;
        std::unordered_map<Entity::ID, Entity::Player> prevPlayerTable;
        std::unordered_map<Entity::ID, PlayerModelState> playerModelStateTable;
        float PLAYER_SPEED = 5.f;

        // Simulation
        Util::Time::SteadyPoint preSimulationTime;
        Util::Time::Seconds simulationLag{0.0};
        Util::Time::Seconds deltaTime{0.0};
        Util::Time::Seconds minFrameInterval{0.0};

        // Networking
        std::string serverDomain;
        uint16_t serverPort = 0;
        ENet::Host host;
        ENet::PeerId serverId = 0;
        uint8_t playerId = 0;
        Util::Time::Seconds serverTickRate{0.0};
        bool connected = false;
        const uint32_t redundantInputs = 3;
        Util::Ring<Networking::Snapshot> snapshotHistory{16};
        Util::Ring<Entity::PlayerState> localPlayerStateHistory{16};

        // Networking - Prediction
        struct Prediction
        {
            InputReq inputReq;
            Entity::PlayerState origin;
            Entity::PlayerState dest;
            Util::Time::SteadyPoint time;
        };
        Util::Ring<Prediction> predictionHistory_{128};
        uint32_t cmdId = 0;
        uint32_t lastAck = 0;
        Util::Time::Seconds predOffset{0};

        const Util::Time::Seconds ERROR_CORRECTION_DURATION{3.0};
        Entity::PlayerState errorCorrectionDiff;
        Util::Time::SteadyPoint errorCorrectionStart;

        // Networking - Entity Interpolation
        const Util::Time::Seconds EXTRAPOLATION_DURATION{0.25};
        Networking::Snapshot extrapolatedSnapshot;
        Util::Time::Seconds offsetTime{0};

        // Rendering
            // Mgr
        Rendering::RenderMgr renderMgr;

            // Shaders
        GL::Shader shader;
        GL::Shader chunkShader;
        GL::Shader quadShader;
        GL::Shader textShader;
        GL::Shader imgShader;

            // Textures
        GL::Texture flashTexture;

            // Meshes
        Rendering::Mesh quad;
        Rendering::Mesh cylinder;
        Rendering::Mesh sphere;
        Rendering::Mesh cube;
        Rendering::Mesh slope;

            // Models
        Game::Models::Player playerAvatar;
        Game::Models::FPS fpsAvatar;

            // Camera
        Rendering::Camera camera_;
        int drawMode = GL_FILL;
        ::App::Client::CameraController camController_;

        // GUI
        InGameGUI inGameGui{*this};

        // Audio
        Audio::AudioMgr* audioMgr = nullptr;

        // Scene
        ::App::Client::Map map_;

        // Config
        GameOptions gameOptions;
    };
}