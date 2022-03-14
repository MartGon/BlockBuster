#pragma once

#include <gui/TextFactory.h>

#include <util/BBTime.h>

#include <gui/PopUp.h>
#include <gui/PopUpMgr.h>

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

        void DebugWindow();
        void RenderStatsWindow();
        void NetworkStatsWindow();

        InGame* inGame;

        // GUI
        GL::VertexArray guiVao;

        // Fonts
        GUI::FontFamily* pixelFont = nullptr;
        GUI::Text text;

        // PopUps
        enum PopUpState
        {
            MENU,
            OPTIONS,
            VIDEO_SETTINGS,
            WARNING,

            MAX
        };
        GUI::PopUpMgr<PopUpState::MAX> puMgr;

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
    };
}