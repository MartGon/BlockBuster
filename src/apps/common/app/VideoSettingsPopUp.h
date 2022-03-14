#pragma once

#include <gui/PopUp.h>
#include <App.h>

#include <Configuration.h>

namespace App
{
    class VideoSettingsPopUp : public GUI::PopUp
    {
    public:
        VideoSettingsPopUp(AppI& app);

        inline void SetOnClose(std::function<void()> onClose)
        {
            this->onClose = onClose;
        }

        void OnDraw() override;
        void OnOpen() override;
        void OnClose() override;
    private:

        std::function<void()> onClose;

        AppI* app = nullptr;
        Configuration::WindowConfig winConfig;
        Configuration::WindowConfig oldWinConfig;
        Configuration::OpenGLConfig glConfig;
    };
}