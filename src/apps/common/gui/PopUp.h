#pragma once

#include <string>
#include <functional>

#include <imgui/imgui.h>

namespace GUI
{
    class PopUp
    {
    public:

        inline bool IsVisible() const
        {
            return isVisible;
        }

        inline void SetVisible(bool visible)
        {
            isVisible = visible;
        }

        inline void SetButtonVisible(bool visible)
        {
            isButtonVisible = visible;
        }

        inline void SetTitle(std::string title)
        {
            this->title = title;
        }

        inline void SetText(std::string text)
        {
            this->text = text;
        }

        inline void SetFlags(ImGuiWindowFlags flags)
        {
            this->flags = flags;
        }

        inline void SetButtonCallback(std::function<void()> cb)
        {
            onButtonPress = cb;
        }

        void Draw();

    private:

        ImGuiWindowFlags flags = ImGuiWindowFlags_None;
        bool isButtonVisible = false;
        bool isVisible = false;
        std::string title;
        std::string text;
        std::function<void()> onButtonPress;
    };
}