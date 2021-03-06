#pragma once

#include <string>
#include <functional>

#include <imgui/imgui.h>

namespace GUI
{
    class PopUp
    {
    public:
        virtual ~PopUp() {}

        inline bool IsVisible() const
        {
            return isVisible;
        }

        inline void SetVisible(bool visible)
        {
            isVisible = visible;
        }

        inline void Close()
        {
            if(isVisible)
            {
                SetVisible(false);
                OnClose();
            }
        }

        inline void Open()
        {
            if(!isVisible)
            {
                SetVisible(true);
                OnOpen();
            }
        }

        inline bool IsCloseable() const
        {
            return isCloseable;
        }

        inline void SetCloseable(bool closeable)
        {
            isCloseable = closeable;
        }

        inline std::string GetTile()
        {
            return this->title;
        }
        
        inline void SetTitle(std::string title)
        {
            this->title = title;
        }

        inline ImGuiWindowFlags GetFlags() const
        {
            return this->flags;
        }

        inline void SetFlags(ImGuiWindowFlags flags)
        {
            this->flags = flags;
        }

        void Draw();
        virtual void OnDraw() = 0;
        virtual void OnOpen() = 0;
        virtual void OnClose() = 0;

    private:

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize;
        
        bool isVisible = false;
        bool isCloseable = false;
        std::string title = "Title";
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
        void OnOpen() override {};
        void OnClose() override {};

    private:

        bool isButtonVisible = false;
        std::string text;
        std::function<void()> onButtonPress;
    };

    class GenericPopUp : public PopUp
    {
    public:

        inline void SetOnDraw(std::function<void()> content)
        {
            this->content = content;
        }

        inline void SetOnOpen(std::function<void()> onOpen)
        {
            this->onOpen = onOpen;
        }

        inline void SetOnClose(std::function<void()> onClose)
        {
            this->onClose = onClose;
        }

        void OnDraw() override;
        void OnOpen() override;
        void OnClose() override;

    private:
        std::function<void()> content;
        std::function<void()> onOpen;
        std::function<void()> onClose;
    };

    class EditTextPopUp : public PopUp
    {
    public:

        inline void SetLabel(std::string label)
        {
            this->label = label;
        }

        inline void SetStringSize(std::size_t size)
        {
            textBuffer.resize(size);
        }

        inline void SetPlaceHolder(std::string str)
        {
            textBuffer = str;
        }

        inline void SetErrorText(std::string err)
        {
            errorText = err;
        }

        inline void SetOnAccept(std::function<bool(std::string)> onAccept)
        {
            this->onAccept = onAccept;
        }

        inline void SetOnCancel(std::function<void()> onCancel)
        {
            this->onCancel = onCancel;
        }

        inline void SetOnOpen(std::function<void()> onOpen)
        {
            this->onOpen = onOpen;
        }

        inline void SetOnClose(std::function<void()> onClose)
        {
            this->onClose = onClose;
        }

        void OnDraw() override;
        void OnOpen() override;
        void OnClose() override;

    private:
        std::string label = "Label";
        std::string textBuffer = "Placeholder";
        std::string errorText;
        
        bool onError = false;

        std::function<bool(std::string)> onAccept = [](std::string){return true;};
        std::function<void()> onCancel;

        std::function<void()> onOpen;
        std::function<void()> onClose;
    };
}