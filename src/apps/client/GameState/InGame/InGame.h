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
#include <rendering/TextureMgr.h>

#include <game/Map.h>
#include <game/CameraController.h>
#include <game/ChunkMeshMgr.h>
#include <game/models/Player.h>
#include <game/models/FPS.h>
#include <game/models/ModelMgr.h>

#include <util/BBTime.h>
#include <util/Ring.h>
#include <util/Table.h>

#include <entity/Player.h>
#include <entity/PlayerController.h>
#include <entity/Match.h>

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
        InGame(Client* client, std::string serverDomain, uint16_t serverPort, std::string map, std::string playerUuid, std::string playerName);
        
        void Start() override;
        void Update() override;
        void Shutdown() override;

        void ApplyVideoOptions(App::Configuration::WindowConfig& winConfig) override;

    private:

        // Update funcs
        void DoUpdate(Util::Time::Seconds deltaTime);
        void OnEnterMatchState(Match::StateType type);
        void UpdateGameMode();
        void OnNewFrame(Util::Time::Seconds deltaTime);

        // Exit
        void Exit();
        void ReturnToMainMenu();

        // Input
        void HandleSDLEvents();
        void UpdateCamera(Entity::PlayerInput input);
        void WeaponRecoil();

        // World
        void InitGameObjects();
        Entity::Player& GetLocalPlayer();
        Entity::ID GetPlayerTeam(Entity::ID playerId);
        World GetWorld();

        // Networking
        void OnPlayerJoin(Entity::ID playerId, Entity::ID teamId, Networking::PlayerSnapshot playerState);
        void OnPlayerLeave(Entity::ID playerId);
        void OnConnectToServer(ENet::PeerId peerId);
        void OnRecvPacket(ENet::PeerId peerId, uint8_t channelId, ENet::RecvPacket packet);
        void OnRecvPacket(Networking::Packet& packet);
        void HandleGameEvent(Event event);
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
        void DrawGameObjects();
        void DrawModeObjects();
        void DrawCollisionBox(const glm::mat4& viewProjMat, Math::Transform box);
        void DrawGUI();
        void Render();

        // Audio
        void UpdateAudio();

        // Map
        void LoadMap(std::filesystem::path mapFolder, std::string fileName);

        // Game Config 
        void LoadGameOptions();
        void ApplyGameOptions(GameOptions options);
        void WriteGameOptions();

        // Players
        struct PlayerModelState
        {
            Animation::Player shootPlayer;
            Animation::Player deathPlayer;
            Math::Transform armsPivot;
            float gScale = 1.0f;
            bool leftFlashActive = false;
            bool rightFlashActive = false;
            bool flagCarrying = false;
        };
        struct ExtraData
        {
            
        };
        std::unordered_map<Entity::ID, Entity::Player> playerTable;
        std::unordered_map<Entity::ID, Entity::Player> prevPlayerTable;
        std::unordered_map<Entity::ID, PlayerModelState> playerModelStateTable;
        std::unordered_map<Entity::ID, ExtraData> playersExtraData;
        enum TeamColors
        {
            TEAM_BLUE, TEAM_RED, NEUTRAL
        };
        enum FFAColors
        {
            FFA_RED, FFA_BLUE, FFA_DARK_GREEN, FFA_GOLD,
            FFA_BLACK, FFA_WHITE, FFA_ORANGE, FFA_PURPLE,
            FFA_BROWN, FFA_LIME, FFA_CYAN, FFA_PINK,
            FFA_GREY, FFA_MAGENTA, FFA_ROSY_BROWN, FFA_AQUA_MARINE
        };
        static glm::vec4 teamColors[3];
        static glm::vec4 ffaColors[16];

        // Local player
        uint8_t playerId = 0;
        Util::Timer respawnTimer;
        Entity::ID killerId = 0;
        float zoomMod = 0.0f;
        const float zoomSpeed = 5.0f;

        // GameObjects
        std::unordered_map<glm::ivec3, Entity::GameObject::State> gameObjectStates; 

        // Match Making data
        std::string playerUuid;
        std::string playerName;

        // Match
        Match match;

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

        const Util::Time::Seconds ERROR_CORRECTION_DURATION{3.0};
        Entity::PlayerState::Transform errorCorrectionDiff;
        Util::Time::SteadyPoint errorCorrectionStart;

        // Networking - Entity Interpolation
        const Util::Time::Seconds EXTRAPOLATION_DURATION{0.25};
        Networking::Snapshot extrapolatedSnapshot;
        Util::Time::Seconds offsetTime{0};

        // Rendering
            // Mgr
        Rendering::RenderMgr renderMgr;

            // Shaders
        GL::Shader renderShader;
        GL::Shader chunkShader;
        GL::Shader billboardShader;
        GL::Shader textShader;
        GL::Shader imgShader;

            // Meshes
        Rendering::Mesh quad;
        Rendering::Mesh cylinder;
        Rendering::Mesh sphere;
        Rendering::Mesh cube;
        Rendering::Mesh slope;

            // Models
        Game::Models::ModelMgr modelMgr;
        Game::Models::Player playerAvatar;
        Game::Models::FPS fpsAvatar;

            // Billboards
        Rendering::TextureID flagIconId;
        Rendering::TextureID redCrossId;
        Rendering::Billboard* flagIcon;
        Rendering::Billboard* redCrossIcon;
        Util::Table<Rendering::Billboard*> wepIcons;

            // Camera
        Rendering::Camera camera_;
        int drawMode = GL_FILL;
        ::App::Client::CameraController camController_;

        // GUI
        InGameGUI inGameGui{*this};

        // Audio
        Audio::AudioMgr* audioMgr = nullptr;

        // Scene
        std::string mapName;
        ::App::Client::Map map_;

        // Config
        GameOptions gameOptions;
        bool exit = false;
    };
}