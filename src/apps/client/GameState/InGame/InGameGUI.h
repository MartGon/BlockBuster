#pragma once

#include <gui/TextFactory.h>
#include <gui/Image.h>
#include <gui/PopUp.h>
#include <gui/PopUpMgr.h>

#include <util/BBTime.h>

#include <gl/Texture.h>

#include <GameState/InGame/InGameFwd.h>

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

        void HUD();
        void UpdateHealth();
        void UpdateArmor();
        std::string GetBoundedValue(int val, int max);
        void UpdateAmmo();
        void UpdateScore();

        void DebugWindow();
        void RenderStatsWindow();
        void NetworkStatsWindow();

        InGame* inGame;

        // GUI
        GL::VertexArray guiVao;

        // Fonts
        GUI::FontFamily* pixelFont = nullptr;

        // PopUps
        GUI::PopUpMgr<PopUpState::MAX> puMgr;

        // Textures
        GL::Texture crosshair;

        // HUD
        GUI::Text healthIcon;
        GUI::Text healthText;
        GUI::Text armorText;
        GUI::Text shieldIcon;
        GUI::Text ammoText;
        GUI::Text ammoNumIcon;
        GUI::Text leftScoreText;
        GUI::Text midScoreText;
        GUI::Text rightScoreText;
        GUI::Image crosshairImg;

        // Game options
        GameOptions gameOptions;

        // Metrics
        double maxFPS = 60.0;

        //TODO: DEBUG. Remove on final version
        // Player Model
        uint32_t modelId = 0;
        float sliderPrecision = 2.0f;
        glm::vec3 modelOffset{0.0f};
        glm::vec3 modelScale{1.0f};
        glm::vec3 modelRot{0.0f};
        glm::ivec2 wtPos{0};
        float tScale = 1.0f;
    };
}