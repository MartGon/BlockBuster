#pragma once

#include <gui/TextFactory.h>
#include <gui/Image.h>
#include <gui/PopUp.h>
#include <gui/PopUpMgr.h>

#include <animation/Animation.h>

#include <util/BBTime.h>
#include <util/Table.h>
#include <gl/Texture.h>
#include <GameState/InGame/InGameFwd.h>
#include <rendering/TextureMgr.h>
#include <entity/Player.h>
#include <mglogger/Logger.h>

namespace BlockBuster
{
    class InGame;

    class InGameGUI
    {
    friend class InGame;

    public:
        InGameGUI(InGame& inGame);

        void Start();
        void DrawGUI(GL::Shader& textShader);

        void OpenMenu();
        void CloseMenu();
        bool IsMenuOpen();

        enum class HitMarkerType
        {
            DMG,
            KILL
        };

        enum ScreenEffect
        {
            SCREEN_EFFECT_DMG,
            SCREEN_EFFECT_HEALING,
            SCREEN_EFFECT_COUNT
        };

        void PlayHitMarkerAnim(HitMarkerType type);
        void PlayScreenEffect(ScreenEffect effect = ScreenEffect::SCREEN_EFFECT_DMG);

        void EnableActionText(Entity::GameObject& go);
        void DisableActionText();
        void EnableScore(bool enabled = true);
        void EnableHUD(bool enabled);
        void EnableWinnerText(bool enabled);
        void EnableCapturingText(bool enabled);
        void SetCapturePercent(float percent);

        void ShowLogMsg(std::string msg);

        glm::vec4 GetOppositeColor(Entity::ID playerTeam);

    private:
        // PopUps
        enum PopUpState
        {
            MENU,
            OPTIONS,
            VIDEO_SETTINGS,
            WARNING,

            MAX
        };
        void OpenMenu(PopUpState state);
        
        void InitPopUps();
        void InitTexts();
        void InitAnimations();

        void HUD();

        void UpdateHealth();
        void UpdateArmor();
        std::string GetBoundedValue(int val, int max);
        std::string LeadingZeros(int val, int size);
        void UpdateAmmo();
        void UpdateScore();
        void UpdateRespawnText();
        void UpdateCountdownText();
        void UpdateGameTimeText();

        void ScoreboardWindow();
        void ScoreTable(const char* name, Entity::ID teamId = 0);
        void RenderStatsWindow();
        void NetworkStatsWindow();

        void DebugWindow();

        // Handy
        Log::Logger* GetLogger();

        InGame* inGame;

        // GUI
        GL::VertexArray guiVao;

        // Fonts
        GUI::FontFamily* pixelFont = nullptr;

        // PopUps
        GUI::PopUpMgr<PopUpState::MAX> puMgr;

        // Textures
        Rendering::TextureID hitmarker;
        Rendering::TextureID dmgTexture;
        Rendering::TextureID grenadeTexId;
        Rendering::TextureID dmgArrowId;
        Util::Table<Rendering::TextureID> crosshairTextures;

        // HUD
        GUI::Text healthIcon;
        GUI::Text healthText;
        GUI::Text armorText;
        GUI::Text shieldIcon;
        GUI::Text ammoText;
        GUI::Text ammoNumIcon;
        GUI::Text grenadeNumText;
        GUI::Text actionText;
        GUI::Image wepIcon;
        GUI::Image altWepIcon;
        GUI::Image grenadeIcon;
        GUI::Image crosshairImg;
        GUI::Image hitmarkerImg;
        GUI::Image flagIconImg;
        GUI::Image dmgEffectImg;
        GUI::Image dmgArrowImg;
        GUI::Image actionImg;

        // Animations
        Animation::Clip hitmarkerAnim;
        bool showHitmarker = false;
        Animation::Player hitMarkerPlayer;
        float dmgAlpha = 0.0f;
        float dmgArrowAlpha = 0.0f;
        const glm::ivec3 effectColor[SCREEN_EFFECT_COUNT] = {glm::vec3{1.0f, 0.0f, 0.0f}, glm::vec3{0.0f, 1.0f, 0.0f}};
        ScreenEffect effectType = SCREEN_EFFECT_DMG;
        Animation::Clip dmgAnim;
        Animation::Player dmgAnimationPlayer;
        Animation::Player dmgArrowAnimationPlayer;

        // Score
        GUI::Text leftScoreText;
        GUI::Text midScoreText;
        GUI::Text rightScoreText;
        GUI::Text gameTimeText;
        GUI::Text winnerText;
        GUI::Text winnerAnnoucerText;

        // Log
        GUI::Text killText;
        GUI::Text respawnTimeText;
        GUI::Text countdownText;
        GUI::Text capturingText;
        GUI::Text captCountdownText;
        GUI::Text logText;
        float logAlpha = 1.0f;
        Animation::Clip logAnim;
        Animation::Player logAnimPlayer;

        // ScoreBoard
        bool showScoreboard = false;

        // Game options
        GameOptions gameOptions;

        // Metrics
        double maxFPS = 60.0;

        // Player Model
    #ifdef _DEBUG
        uint32_t modelId = 0;
        float sliderPrecision = 2.0f;
        glm::vec3 modelOffset{0.0f};
        glm::vec3 modelScale{1.0f};
        glm::vec3 modelRot{0.0f};
        glm::ivec2 wtPos{0};
        float tScale = 0.45f;
        bool tShow = false;
    #endif
    };
}