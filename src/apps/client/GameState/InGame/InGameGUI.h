#pragma once

#include <gui/TextFactory.h>

#include <util/BBTime.h>

#include <gui/PopUp.h>
#include <gui/PopUpMgr.h>

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
            WARNING,

            MAX
        };
        GUI::PopUpMgr<PopUpState::MAX> puMgr;

        // Game options
        float sensitivity = 1.0f;
        bool sound = true;

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