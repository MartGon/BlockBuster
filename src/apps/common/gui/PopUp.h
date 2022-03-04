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

        inline bool IsCloseable() const
        {
            return isCloseable;
        }

        inline void SetCloseable(bool closeable)
        {
            isCloseable = closeable;
        }

        
        inline void SetTitle(std::string title)
        {
            this->title = title;
        }

        inline void SetFlags(ImGuiWindowFlags flags)
        {
            this->flags = flags;
        }

        void Draw();
        virtual void OnDraw() = 0;

    private:

        ImGuiWindowFlags flags = ImGuiWindowFlags_None;
        
        bool isVisible = false;
        bool isCloseable = false;
        std::string title;
    };

    class BasicPopUp : public PopUp
    {
    public:
        inline void SetButtonVisible(bool visible)
        {
            isButtonVisible = visible;
        }

        inline void SetButtonCallback(std::function<void()> cb)
        {
            onButtonPress = cb;
        }

        inline void SetText(std::string text)
        {
            this->text = text;
        }

        void OnDraw() override; 

    private:

        bool isButtonVisible = false;
        std::string text;
        std::function<void()> onButtonPress;
    };

    class GenericPopUp : public PopUp
    {
    public:
        GenericPopUp(std::function<void()> content) : content{content} {}

        void OnDraw() override;

    private:
        std::function<void()> content;
    };
}